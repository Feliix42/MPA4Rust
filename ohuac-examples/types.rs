// simply stolen from servo

pub enum WindowEvent {
    /// Sent when no message has arrived, but the event loop was kicked for some reason (perhaps
    /// by another Servo subsystem).
    Idle,
    /// Sent when part of the window is marked dirty and needs to be redrawn. Before sending this
    /// message, the window must make the same GL context as in `PrepareRenderingEvent` current.
    Refresh,
    /// Sent to initialize the GL context. The windowing system must have a valid, current GL
    /// context when this message is sent.
    InitializeCompositing,
    /// Sent when the window is resized.
    Resize(TypedSize2D<u32, DevicePixel>),
    /// Touchpad Pressure
    TouchpadPressure(TypedPoint2D<f32, DevicePixel>, f32, TouchpadPressurePhase),
    /// Sent when you want to override the viewport.
    Viewport(TypedPoint2D<u32, DevicePixel>, TypedSize2D<u32, DevicePixel>),
    /// Sent when a new URL is to be loaded.
    LoadUrl(String),
    /// Sent when a mouse hit test is to be performed.
    MouseWindowEventClass(MouseWindowEvent),
    /// Sent when a mouse move.
    MouseWindowMoveEventClass(TypedPoint2D<f32, DevicePixel>),
    /// Touch event: type, identifier, point
    Touch(TouchEventType, TouchId, TypedPoint2D<f32, DevicePixel>),
    /// Sent when the user scrolls. The first point is the delta and the second point is the
    /// origin.
    Scroll(ScrollLocation, TypedPoint2D<i32, DevicePixel>, TouchEventType),
    /// Sent when the user zooms.
    Zoom(f32),
    /// Simulated "pinch zoom" gesture for non-touch platforms (e.g. ctrl-scrollwheel).
    PinchZoom(f32),
    /// Sent when the user resets zoom to default.
    ResetZoom,
    /// Sent when the user uses chrome navigation (i.e. backspace or shift-backspace).
    Navigation(WindowNavigateMsg),
    /// Sent when the user quits the application
    Quit,
    /// Sent when a key input state changes
    KeyEvent(Option<char>, Key, KeyState, KeyModifiers),
    /// Sent when Ctr+R/Apple+R is called to reload the current page.
    Reload,
}
