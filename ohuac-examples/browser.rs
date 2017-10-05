pub struct IOCompositor<Window: WindowMethods> {
    /// The application window.
    window: Rc<Window>,

    /// The port on which we receive messages.
    port: Box<CompositorReceiver>,

    /// The root pipeline.
    root_pipeline: Option<CompositionPipeline>,

    /// Tracks details about each active pipeline that the compositor knows about.
    pipeline_details: HashMap<PipelineId, PipelineDetails>,

    /// The scene scale, to allow for zooming and high-resolution painting.
    scale: ScaleFactor<f32, LayerPixel, DevicePixel>,

    /// The size of the rendering area.
    frame_size: TypedSize2D<u32, DevicePixel>,

    /// The position and size of the window within the rendering area.
    window_rect: TypedRect<u32, DevicePixel>,

    /// The overridden viewport.
    viewport: Option<(TypedPoint2D<u32, DevicePixel>, TypedSize2D<u32, DevicePixel>)>,

    /// "Mobile-style" zoom that does not reflow the page.
    viewport_zoom: PinchZoomFactor,

    /// Viewport zoom constraints provided by @viewport.
    min_viewport_zoom: Option<PinchZoomFactor>,
    max_viewport_zoom: Option<PinchZoomFactor>,

    /// "Desktop-style" zoom that resizes the viewport to fit the window.
    page_zoom: ScaleFactor<f32, CSSPixel, DeviceIndependentPixel>,

    /// The device pixel ratio for this window.
    scale_factor: ScaleFactor<f32, DeviceIndependentPixel, DevicePixel>,

    channel_to_self: Box<CompositorProxy + Send>,

    /// A handle to the delayed composition timer.
    delayed_composition_timer: DelayedCompositionTimerProxy,

    /// The type of composition to perform
    composite_target: CompositeTarget,

    /// Tracks whether we should composite this frame.
    composition_request: CompositionRequest,

    /// Tracks whether we are in the process of shutting down, or have shut down and should close
    /// the compositor.
    shutdown_state: ShutdownState,

    /// Tracks the last composite time.
    last_composite_time: u64,

    /// Tracks whether the zoom action has happened recently.
    zoom_action: bool,

    /// The time of the last zoom action has started.
    zoom_time: f64,

    /// Whether the page being rendered has loaded completely.
    /// Differs from ReadyState because we can finish loading (ready)
    /// many times for a single page.
    got_load_complete_message: bool,

    /// The current frame tree ID (used to reject old paint buffers)
    frame_tree_id: FrameTreeId,

    /// The channel on which messages can be sent to the constellation.
    constellation_chan: Sender<ConstellationMsg>,

    /// The channel on which messages can be sent to the time profiler.
    time_profiler_chan: time::ProfilerChan,

    /// Touch input state machine
    touch_handler: TouchHandler,

    /// Pending scroll/zoom events.
    pending_scroll_zoom_events: Vec<ScrollZoomEvent>,

    /// Whether we're waiting on a recomposite after dispatching a scroll.
    waiting_for_results_of_scroll: bool,

    /// Used by the logic that determines when it is safe to output an
    /// image for the reftest framework.
    ready_to_save_state: ReadyState,

    /// Whether a scroll is in progress; i.e. whether the user's fingers are down.
    scroll_in_progress: bool,

    in_scroll_transaction: Option<Instant>,

    /// The webrender renderer.
    webrender: webrender::Renderer,

    /// The webrender interface, if enabled.
    webrender_api: webrender_traits::RenderApi,

    /// GL functions interface (may be GL or GLES)
    gl: Rc<gl::Gl>,
}

pub struct ConstellationData {
    top_level_browsing_context_id: TopLevelBrowsingContextId,
    data: WindowSizeData, 
    size_type: WindowSizeType
}


impl IOCompositor {
    // marks stateful functions
    #[sfn]
    fn on_resize_window_event(&mut self, new_size: TypedSize2D<u32, DevicePixel>) -> (webrender_traits::DeviceUintRect, webrender_traits::DeviceUintSize, ConstellationData) {
        debug!("compositor resizing to {:?}", new_size.to_untyped());

        // A size change could also mean a resolution change.
        let new_scale_factor = self.window.hidpi_factor();
        if self.scale_factor != new_scale_factor {
            self.scale_factor = new_scale_factor;
            self.update_zoom_transform();
        }

        let new_window_rect = self.window.window_rect();
        let new_frame_size = self.window.framebuffer_size();

        if self.window_rect == new_window_rect &&
           self.frame_size == new_frame_size {
            return;
        }

        self.frame_size = new_size;
        self.window_rect = new_window_rect;

        self.set_window_size(WindowSizeType::Resize)
    }


    // renamed from send_window_size()
    #[sfn]
    fn set_window_size(&self, size_type: WindowSizeType) -> (webrender_traits::DeviceUintRect, webrender_traits::DeviceUintSize, ConstellationData) {
        let dppx = self.page_zoom * self.hidpi_factor();

        let window_rect = {
            let offset = webrender_traits::DeviceUintPoint::new(self.window_rect.origin.x, self.window_rect.origin.y);
            let size = webrender_traits::DeviceUintSize::new(self.window_rect.size.width, self.window_rect.size.height);
            webrender_traits::DeviceUintRect::new(offset, size)
        };

        let frame_size = webrender_traits::DeviceUintSize::new(self.frame_size.width, self.frame_size.height);

        let initial_viewport = self.window_rect.size.to_f32() / dppx;

        let data = WindowSizeData {
            device_pixel_ratio: dppx,
            initial_viewport: initial_viewport,
        };
        let top_level_browsing_context_id = match self.root_pipeline {
            Some(ref pipeline) => pipeline.top_level_browsing_context_id,
            None => return warn!("Window resize without root pipeline."),
        };
        let constellation_data = ConstellationData {top_level_browsing_context_id, data, size_type};

        (window_rect, frame_size, constellation_data)
    }
}



pub struct Constellation<Message, LTF, STF> {
    /// An IPC channel for script threads to send messages to the constellation.
    /// This is the script threads' view of `script_receiver`.
    script_sender: IpcSender<FromScriptMsg>,

    /// A channel for the constellation to receive messages from script threads.
    /// This is the constellation's view of `script_sender`.
    script_receiver: Receiver<Result<FromScriptMsg, IpcError>>,

    /// An IPC channel for layout threads to send messages to the constellation.
    /// This is the layout threads' view of `layout_receiver`.
    layout_sender: IpcSender<FromLayoutMsg>,

    /// A channel for the constellation to receive messages from layout threads.
    /// This is the constellation's view of `layout_sender`.
    layout_receiver: Receiver<Result<FromLayoutMsg, IpcError>>,

    /// A channel for the constellation to receive messages from the compositor thread.
    compositor_receiver: Receiver<FromCompositorMsg>,

    /// A channel (the implementation of which is port-specific) for the
    /// constellation to send messages to the compositor thread.
    compositor_proxy: Box<CompositorProxy>,

    /// Channels for the constellation to send messages to the public
    /// resource-related threads.  There are two groups of resource
    /// threads: one for public browsing, and one for private
    /// browsing.
    public_resource_threads: ResourceThreads,

    /// Channels for the constellation to send messages to the private
    /// resource-related threads.  There are two groups of resource
    /// threads: one for public browsing, and one for private
    /// browsing.
    private_resource_threads: ResourceThreads,

    /// A channel for the constellation to send messages to the font
    /// cache thread.
    font_cache_thread: FontCacheThread,

    /// A channel for the constellation to send messages to the
    /// debugger thread.
    debugger_chan: Option<debugger::Sender>,

    /// A channel for the constellation to send messages to the
    /// devtools thread.
    devtools_chan: Option<Sender<DevtoolsControlMsg>>,

    /// An IPC channel for the constellation to send messages to the
    /// bluetooth thread.
    bluetooth_thread: IpcSender<BluetoothRequest>,

    /// An IPC channel for the constellation to send messages to the
    /// Service Worker Manager thread.
    swmanager_chan: Option<IpcSender<ServiceWorkerMsg>>,

    /// An IPC channel for Service Worker Manager threads to send
    /// messages to the constellation.  This is the SW Manager thread's
    /// view of `swmanager_receiver`.
    swmanager_sender: IpcSender<SWManagerMsg>,

    /// A channel for the constellation to receive messages from the
    /// Service Worker Manager thread. This is the constellation's view of
    /// `swmanager_sender`.
    swmanager_receiver: Receiver<Result<SWManagerMsg, IpcError>>,

    /// A channel for the constellation to send messages to the
    /// time profiler thread.
    time_profiler_chan: time::ProfilerChan,

    /// A channel for the constellation to send messages to the
    /// memory profiler thread.
    mem_profiler_chan: mem::ProfilerChan,

    /// A channel for the constellation to send messages to the
    /// timer thread.
    scheduler_chan: IpcSender<TimerSchedulerMsg>,

    /// A channel for the constellation to send messages to the
    /// Webrender thread.
    webrender_api_sender: webrender_traits::RenderApiSender,

    /// The set of all event loops in the browser. We generate a new
    /// event loop for each registered domain name (aka eTLD+1) in
    /// each top-level browsing context. We store the event loops in a map
    /// indexed by top-level browsing context id
    /// (as a `TopLevelBrowsingContextId`) and registered
    /// domain name (as a `Host`) to event loops. This double
    /// indirection ensures that separate tabs do not share event
    /// loops, even if the same domain is loaded in each.
    /// It is important that scripts with the same eTLD+1
    /// share an event loop, since they can use `document.domain`
    /// to become same-origin, at which point they can share DOM objects.
    event_loops: HashMap<TopLevelBrowsingContextId, HashMap<Host, Weak<EventLoop>>>,

    /// The set of all the pipelines in the browser.
    /// (See the `pipeline` module for more details.)
    pipelines: HashMap<PipelineId, Pipeline>,

    /// The set of all the browsing contexts in the browser.
    browsing_contexts: HashMap<BrowsingContextId, BrowsingContext>,

    /// When a navigation is performed, we do not immediately update
    /// the session history, instead we ask the event loop to begin loading
    /// the new document, and do not update the browsing context until the
    /// document is active. Between starting the load and it activating,
    /// we store a `SessionHistoryChange` object for the navigation in progress.
    pending_changes: Vec<SessionHistoryChange>,

    /// The root browsing context.
    root_browsing_context_id: TopLevelBrowsingContextId,

    /// The currently focused pipeline for key events.
    focus_pipeline_id: Option<PipelineId>,

    /// Pipeline IDs are namespaced in order to avoid name collisions,
    /// and the namespaces are allocated by the constellation.
    next_pipeline_namespace_id: PipelineNamespaceId,

    /// The size of the top-level window.
    window_size: WindowSizeData,

    /// Bits of state used to interact with the webdriver implementation
    webdriver: WebDriverData,

    /// Document states for loaded pipelines (used only when writing screenshots).
    document_states: HashMap<PipelineId, DocumentState>,

    /// Are we shutting down?
    shutting_down: bool,

    /// Have we seen any warnings? Hopefully always empty!
    /// The buffer contains `(thread_name, reason)` entries.
    handled_warnings: VecDeque<(Option<String>, String)>,

    /// The random number generator and probability for closing pipelines.
    /// This is for testing the hardening of the constellation.
    random_pipeline_closure: Option<(ServoRng, f32)>,

    /// Phantom data that keeps the Rust type system happy.
    phantom: PhantomData<(Message, LTF, STF)>,

    /// A channel through which messages can be sent to the webvr thread.
    webvr_thread: Option<IpcSender<WebVRMsg>>,
}


impl Constellation {
    #[sfn]
    fn handle_window_size_msg(&mut self,
                              top_level_browsing_context_id: TopLevelBrowsingContextId,
                              new_size: WindowSizeData,
                              size_type: WindowSizeType) -> (Option<Pipeline>, Option<FilterMap<Pipeline>>, Vec<Pipeline>)
    {
        debug!("handle_window_size_msg: {:?}", new_size.initial_viewport.to_untyped());

        self.window_size = new_size;

        let browsing_context_id = BrowsingContextId::from(top_level_browsing_context_id);
        self.resize_browsing_context(new_size, size_type, browsing_context_id);
    }

    #[sfn]
    fn resize_browsing_context(&mut self,
                               new_size: WindowSizeData,
                               size_type: WindowSizeType,
                               browsing_context_id: BrowsingContextId) -> (Option<Pipeline>, Option<FilterMap<Pipeline>>, Vec<Pipeline>)
    {
        if let Some(browsing_context) = self.browsing_contexts.get_mut(&browsing_context_id) {
            browsing_context.size = Some(new_size.initial_viewport);
        }

        let mut future_pipelines = Vec::new();

        // Send resize message to any pending pipelines that aren't loaded yet.
        for change in &self.pending_changes {
            let pipeline_id = change.new_pipeline_id;
            let pipeline = match self.pipelines.get(&pipeline_id) {
                None => { warn!("Pending pipeline {:?} is closed", pipeline_id); continue; }
                Some(pipeline) => pipeline,
            };
            if pipeline.browsing_context_id == browsing_context_id {
                future_pipelines.push(pipeline);
            }
        }

        if let Some(browsing_context) = self.browsing_contexts.get(&browsing_context_id) {
            // Send Resize (or ResizeInactive) messages to each
            // pipeline in the frame tree.
            let pipeline_id = browsing_context.pipeline_id;
            let pipeline = match self.pipelines.get(&pipeline_id) {
                None => return warn!("Pipeline {:?} resized after closing.", pipeline_id),
                Some(pipeline) => pipeline,
            };

            let pipelines = browsing_context.prev.iter().chain(browsing_context.next.iter())
                .filter_map(|entry| entry.pipeline_id)
                .filter_map(|pipeline_id| self.pipelines.get(&pipeline_id));

            // return active pipeline, inactive pipelines and pending pipelines
            return (Some(pipeline), pipelines, future_pipelines));
        }

    (None, None, future_pipelines)
    }
}



pub struct RenderBackend {
    api_rx: MsgReceiver<ApiMsg>,
    payload_rx: PayloadReceiver,
    payload_tx: PayloadSender,
    result_tx: Sender<ResultMsg>,

    // TODO(gw): Consider using strongly typed units here.
    hidpi_factor: f32,
    page_zoom_factor: f32,
    pinch_zoom_factor: f32,
    pan: DeviceIntPoint,
    window_size: DeviceUintSize,
    inner_rect: DeviceUintRect,
    next_namespace_id: IdNamespace,

    resource_cache: ResourceCache,

    scene: Scene,
    frame: Frame,

    notifier: Arc<Mutex<Option<Box<RenderNotifier>>>>,
    webrender_context_handle: Option<GLContextHandleWrapper>,
    webgl_contexts: HashMap<WebGLContextId, GLContextWrapper>,
    current_bound_webgl_context_id: Option<WebGLContextId>,
    recorder: Option<Box<ApiRecordingReceiver>>,
    main_thread_dispatcher: Arc<Mutex<Option<Box<RenderDispatcher>>>>,

    next_webgl_id: usize,

    vr_compositor_handler: Arc<Mutex<Option<Box<VRCompositorHandler>>>>
}

impl RenderBackend {
    #[sfn]
    pub fn set_window_parameters(&mut self, window_size: webrender_traits::DeviceUintSize, inner_rect: webrender_traits::DeviceUintRect) => {
        self.window_size = window_size;
        self.inner_rect = inner_rect;
    }
}


#[derive(JSTraceable)]
// ScriptThread instances are rooted on creation, so this is okay
#[allow(unrooted_must_root)]
pub struct ScriptThread {
    /// The documents for pipelines managed by this thread
    documents: DOMRefCell<Documents>,
    /// The window proxies known by this thread
    /// TODO: this map grows, but never shrinks. Issue #15258.
    window_proxies: DOMRefCell<HashMap<BrowsingContextId, JS<WindowProxy>>>,
    /// A list of data pertaining to loads that have not yet received a network response
    incomplete_loads: DOMRefCell<Vec<InProgressLoad>>,
    /// A map to store service worker registrations for a given origin
    registration_map: DOMRefCell<HashMap<ServoUrl, JS<ServiceWorkerRegistration>>>,
    /// A job queue for Service Workers keyed by their scope url
    job_queue_map: Rc<JobQueue>,
    /// Image cache for this script thread.
    image_cache: Arc<ImageCache>,
    /// A handle to the resource thread. This is an `Arc` to avoid running out of file descriptors if
    /// there are many iframes.
    resource_threads: ResourceThreads,
    /// A handle to the bluetooth thread.
    bluetooth_thread: IpcSender<BluetoothRequest>,

    /// The port on which the script thread receives messages (load URL, exit, etc.)
    port: Receiver<MainThreadScriptMsg>,
    /// A channel to hand out to script thread-based entities that need to be able to enqueue
    /// events in the event queue.
    chan: MainThreadScriptChan,

    dom_manipulation_task_source: DOMManipulationTaskSource,

    user_interaction_task_source: UserInteractionTaskSource,

    networking_task_source: NetworkingTaskSource,

    history_traversal_task_source: HistoryTraversalTaskSource,

    file_reading_task_source: FileReadingTaskSource,

    /// A channel to hand out to threads that need to respond to a message from the script thread.
    control_chan: IpcSender<ConstellationControlMsg>,

    /// The port on which the constellation and layout threads can communicate with the
    /// script thread.
    control_port: Receiver<ConstellationControlMsg>,

    /// For communicating load url messages to the constellation
    constellation_chan: IpcSender<ConstellationMsg>,

    /// A sender for new layout threads to communicate to the constellation.
    layout_to_constellation_chan: IpcSender<LayoutMsg>,

    /// The port on which we receive messages from the image cache
    image_cache_port: Receiver<ImageCacheMsg>,

    /// The channel on which the image cache can send messages to ourself.
    image_cache_channel: Sender<ImageCacheMsg>,
    /// For providing contact with the time profiler.
    time_profiler_chan: time::ProfilerChan,

    /// For providing contact with the memory profiler.
    mem_profiler_chan: mem::ProfilerChan,

    /// For providing instructions to an optional devtools server.
    devtools_chan: Option<IpcSender<ScriptToDevtoolsControlMsg>>,
    /// For receiving commands from an optional devtools server. Will be ignored if
    /// no such server exists.
    devtools_port: Receiver<DevtoolScriptControlMsg>,
    devtools_sender: IpcSender<DevtoolScriptControlMsg>,

    /// The JavaScript runtime.
    js_runtime: Rc<Runtime>,

    /// The topmost element over the mouse.
    topmost_mouse_over_target: MutNullableJS<Element>,

    /// List of pipelines that have been owned and closed by this script thread.
    closed_pipelines: DOMRefCell<HashSet<PipelineId>>,

    scheduler_chan: IpcSender<TimerSchedulerMsg>,
    timer_event_chan: Sender<TimerEvent>,
    timer_event_port: Receiver<TimerEvent>,

    content_process_shutdown_chan: IpcSender<()>,

    microtask_queue: MicrotaskQueue,

    /// Microtask Queue for adding support for mutation observer microtasks
    mutation_observer_compound_microtask_queued: Cell<bool>,

    /// The unit of related similar-origin browsing contexts' list of MutationObserver objects
    mutation_observers: DOMRefCell<Vec<JS<MutationObserver>>>,

    /// A handle to the webvr thread, if available
    webvr_thread: Option<IpcSender<WebVRMsg>>,

    /// The worklet thread pool
    worklet_thread_pool: DOMRefCell<Option<Rc<WorkletThreadPool>>>,

    /// A list of pipelines containing documents that finished loading all their blocking
    /// resources during a turn of the event loop.
    docs_with_no_blocking_loads: DOMRefCell<HashSet<JS<Document>>>,

    /// A list of nodes with in-progress CSS transitions, which roots them for the duration
    /// of the transition.
    transitioning_nodes: DOMRefCell<Vec<JS<Node>>>,
}

impl ScriptThread {
    #[sfn]
    fn handle_resize(&self, id: PipelineId, size: WindowSizeData, size_type: WindowSizeType) {
        let window = self.documents.borrow().find_window(id);
        if let Some(ref window) = window {
            window.set_resize_event(size, size_type);
            return;
        }
        let mut loads = self.incomplete_loads.borrow_mut();
        if let Some(ref mut load) = loads.iter_mut().find(|load| load.pipeline_id == id) {
            load.window_size = Some(size);
            return;
        }
        warn!("resize sent to nonexistent pipeline");
    }

    /// Window was resized, but this script was not active, so don't reflow yet
    fn handle_resize_inactive_msg(&self, id: PipelineId, new_size: WindowSizeData) {
        let window = self.documents.borrow().find_window(id)
            .expect("ScriptThread: received a resize msg for a pipeline not in this script thread. This is a bug.");
        window.set_window_size(new_size);
    }
}


#[dom_struct]
pub struct Window {
    globalscope: GlobalScope,
    #[ignore_heap_size_of = "trait objects are hard"]
    script_chan: MainThreadScriptChan,
    #[ignore_heap_size_of = "task sources are hard"]
    dom_manipulation_task_source: DOMManipulationTaskSource,
    #[ignore_heap_size_of = "task sources are hard"]
    user_interaction_task_source: UserInteractionTaskSource,
    #[ignore_heap_size_of = "task sources are hard"]
    networking_task_source: NetworkingTaskSource,
    #[ignore_heap_size_of = "task sources are hard"]
    history_traversal_task_source: HistoryTraversalTaskSource,
    #[ignore_heap_size_of = "task sources are hard"]
    file_reading_task_source: FileReadingTaskSource,
    navigator: MutNullableJS<Navigator>,
    #[ignore_heap_size_of = "Arc"]
    image_cache: Arc<ImageCache>,
    #[ignore_heap_size_of = "channels are hard"]
    image_cache_chan: Sender<ImageCacheMsg>,
    window_proxy: MutNullableJS<WindowProxy>,
    document: MutNullableJS<Document>,
    history: MutNullableJS<History>,
    performance: MutNullableJS<Performance>,
    navigation_start: u64,
    navigation_start_precise: f64,
    screen: MutNullableJS<Screen>,
    session_storage: MutNullableJS<Storage>,
    local_storage: MutNullableJS<Storage>,
    status: DOMRefCell<DOMString>,

    /// For sending timeline markers. Will be ignored if
    /// no devtools server
    devtools_markers: DOMRefCell<HashSet<TimelineMarkerType>>,
    #[ignore_heap_size_of = "channels are hard"]
    devtools_marker_sender: DOMRefCell<Option<IpcSender<Option<TimelineMarker>>>>,

    /// Pending resize event, if any.
    resize_event: Cell<Option<(WindowSizeData, WindowSizeType)>>,

    /// Parent id associated with this page, if any.
    parent_info: Option<(PipelineId, FrameType)>,

    /// Global static data related to the DOM.
    dom_static: GlobalStaticData,

    /// The JavaScript runtime.
    #[ignore_heap_size_of = "Rc<T> is hard"]
    js_runtime: DOMRefCell<Option<Rc<Runtime>>>,

    /// A handle for communicating messages to the layout thread.
    #[ignore_heap_size_of = "channels are hard"]
    layout_chan: Sender<Msg>,

    /// A handle to perform RPC calls into the layout, quickly.
    #[ignore_heap_size_of = "trait objects are hard"]
    layout_rpc: Box<LayoutRPC + Send + 'static>,

    /// The current size of the window, in pixels.
    window_size: Cell<Option<WindowSizeData>>,

    /// A handle for communicating messages to the bluetooth thread.
    #[ignore_heap_size_of = "channels are hard"]
    bluetooth_thread: IpcSender<BluetoothRequest>,

    bluetooth_extra_permission_data: BluetoothExtraPermissionData,

    /// An enlarged rectangle around the page contents visible in the viewport, used
    /// to prevent creating display list items for content that is far away from the viewport.
    page_clip_rect: Cell<Rect<Au>>,

    /// Flag to suppress reflows. The first reflow will come either with
    /// RefreshTick or with FirstLoad. Until those first reflows, we want to
    /// suppress others like MissingExplicitReflow.
    suppress_reflow: Cell<bool>,

    /// A counter of the number of pending reflows for this window.
    pending_reflow_count: Cell<u32>,

    /// A channel for communicating results of async scripts back to the webdriver server
    #[ignore_heap_size_of = "channels are hard"]
    webdriver_script_chan: DOMRefCell<Option<IpcSender<WebDriverJSResult>>>,

    /// The current state of the window object
    current_state: Cell<WindowState>,

    current_viewport: Cell<Rect<Au>>,

    /// A flag to prevent async events from attempting to interact with this window.
    #[ignore_heap_size_of = "defined in std"]
    ignore_further_async_events: DOMRefCell<Arc<AtomicBool>>,

    error_reporter: CSSErrorReporter,

    /// A list of scroll offsets for each scrollable element.
    scroll_offsets: DOMRefCell<HashMap<UntrustedNodeAddress, Point2D<f32>>>,

    /// All the MediaQueryLists we need to update
    media_query_lists: WeakMediaQueryListVec,

    test_runner: MutNullableJS<TestRunner>,

    /// A handle for communicating messages to the webvr thread, if available.
    #[ignore_heap_size_of = "channels are hard"]
    webvr_thread: Option<IpcSender<WebVRMsg>>,

    /// A map for storing the previous permission state read results.
    permission_state_invocation_results: DOMRefCell<HashMap<String, PermissionState>>,

    /// All of the elements that have an outstanding image request that was
    /// initiated by layout during a reflow. They are stored in the script thread
    /// to ensure that the element can be marked dirty when the image data becomes
    /// available at some point in the future.
    pending_layout_images: DOMRefCell<HashMap<PendingImageId, Vec<JS<Node>>>>,

    /// Directory to store unminified scripts for this window if unminify-js
    /// opt is enabled.
    unminified_js_dir: DOMRefCell<Option<String>>,

    /// Worklets
    test_worklet: MutNullableJS<Worklet>,
}

impl Window {
    #[sfn]
    pub fn set_window_size(&self, size: WindowSizeData) {
        self.window_size.set(Some(size));
    }

    #[sfn]
    pub fn set_resize_event(&self, event: WindowSizeData, event_type: WindowSizeType) {
        self.resize_event.set(Some((event, event_type)));
    }
}
