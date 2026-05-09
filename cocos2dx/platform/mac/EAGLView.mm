/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

/*
 * Idea of subclassing NSOpenGLView was taken from  "TextureUpload" Apple's sample
 */

#import <Availability.h>

#import "EAGLView.h"
#import "CCEGLView.h"
#import <OpenGL/gl.h>
#import "CCDirector.h"
#import "ccConfig.h"
#import "CCSet.h"
#import "CCTouch.h"
#import "CCIMEDispatcher.h"
#import "CCWindow.h"
#import "CCEventDispatcher.h"
#import "CCEGLView.h"

#define CC_USE_IMGUI 0

// imgui
#if CC_USE_IMGUI > 0
#include "imgui.h"
#include "imgui_impl_osx.h"
#include "imgui_impl_cocos2dx_v2.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#define DISPATCH_EVENT_TO_IMGUI(__event__)  \
do {    \
    ImGui_ImplOSX_HandleEvent(__event__, self);  \
    bool inImGuiWidgets = ImGui::IsAnyWindowHovered();  \
    if (inImGuiWidgets) \
    {   \
        return ;    \
    }   \
} while(0);
#else
#define DISPATCH_EVENT_TO_IMGUI(__event__) do {} while(0);
#endif // CC_USE_IMGUI


//USING_NS_CC;
static EAGLView *view;

static void SyncCocosFrameFromNSView(NSView *nsView)
{
	if (!nsView)
		return;
	NSRect b = [nsView bounds];
	cocos2d::CCEGLView *eglView = cocos2d::CCEGLView::sharedOpenGLView();
	if (eglView)
	{
		eglView->handleScreenResize((float)b.size.width, (float)b.size.height);
	}
}

@implementation EAGLView

@synthesize eventDelegate = eventDelegate_, isFullScreen = isFullScreen_, frameZoomFactor=frameZoomFactor_;

+(id) sharedEGLView
{
	return view;
}

- (id) initWithFrame:(NSRect)frameRect
{
	self = [self initWithFrame:frameRect shareContext:nil];
	return self;
}

- (id) initWithFrame:(NSRect)frameRect shareContext:(NSOpenGLContext*)context
{
    NSOpenGLPixelFormatAttribute attribs[] =
    {
//		NSOpenGLPFAAccelerated,
//		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		
		0
    };
	
	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	
	if (!pixelFormat)
		NSLog(@"No OpenGL pixel format");
	
	if( (self = [super initWithFrame:frameRect pixelFormat:[pixelFormat autorelease]]) ) {
		
		if( context )
			[self setOpenGLContext:context];

		// Force the OpenGL context to be created and made current immediately so that
		// any GL calls performed during AppDelegate::applicationDidFinishLaunching
		// (e.g. CCShaderCache loading default shaders from CCDirector::setOpenGLView)
		// have a valid current context. Without this, glCreateProgram() returns 0 and
		// CCGLProgram::link asserts on macOS because prepareOpenGL is normally invoked
		// only right before the first draw.
		NSOpenGLContext *glContext = [self openGLContext];
		if (glContext) {
			[glContext makeCurrentContext];
		}

		// event delegate
		eventDelegate_ = [CCEventDispatcher sharedDispatcher];
	}
    
    cocos2d::CCEGLView::sharedOpenGLView()->setFrameSize(frameRect.size.width, frameRect.size.height);
    
    frameZoomFactor_ = 1.0f;
    mousePressed = false;
	
	view = self;
    
#if CC_USE_IMGUI > 0
    [self setupImGui];
#endif // #if CC_USE_IMGUI > 0
    
	return self;
}

- (id) initWithFrame:(NSRect)frameRect pixelFormat:(NSOpenGLPixelFormat *)format{
    // event delegate
    eventDelegate_ = [CCEventDispatcher sharedDispatcher];
    
    cocos2d::CCEGLView::sharedOpenGLView()->setFrameSize(frameRect.size.width, frameRect.size.height);
    
    frameZoomFactor_ = 1.0f;
    mousePressed = false;
	
	view = self;
    
    [super initWithFrame:frameRect pixelFormat:format];

    // See note in initWithFrame:shareContext: -- the GL context must be current
    // before any cocos2d-x GL initialization runs.
    NSOpenGLContext *glContext = [self openGLContext];
    if (glContext) {
        [glContext makeCurrentContext];
    }

#if CC_USE_IMGUI > 0
    [self setupImGui];
#endif // #if CC_USE_IMGUI > 0
    
    return self;
}

- (void) update
{
	// XXX: Should I do something here ?
	[super update];
}

- (void) prepareOpenGL
{
	// XXX: Initialize OpenGL context

	[super prepareOpenGL];
	
	// Make this openGL context current to the thread
	// (i.e. all openGL on this thread calls will go to this context)
	[[self openGLContext] makeCurrentContext];
	
	// Synchronize buffer swaps with vertical refresh rate
	GLint swapInt = 1;
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];	

//	GLint order = -1;
//	[[self openGLContext] setValues:&order forParameter:NSOpenGLCPSurfaceOrder];
}

- (NSUInteger) depthFormat
{
	return 24;
}

- (void) setFrameZoomFactor:(float)frameZoomFactor
{
    frameZoomFactor_ = frameZoomFactor;
    
    NSRect winRect = [[self window] frame];
    NSRect viewRect = [self frame];
    
    // compute the margin width and margin height
    float diffX = winRect.size.width - viewRect.size.width;
    float diffY = winRect.size.height - viewRect.size.height;
    
    // new window width and height
    float newWindowWidth = (int)(viewRect.size.width * frameZoomFactor + diffX);
    float newWindowHeight = (int)(viewRect.size.height * frameZoomFactor + diffY);
    
    // display window in the center of the screen
    NSRect screenRect = [[NSScreen mainScreen] frame];
    float originX = (screenRect.size.width - newWindowWidth) / 2;
    float originY = (screenRect.size.height - newWindowHeight) / 2;
    
    [[self window] setFrame:NSMakeRect(originX, originY, newWindowWidth, newWindowHeight) display:true];
}

- (void) reshape
{
	// We draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main thread
	// Add a mutex around to avoid the threads accessing the context simultaneously when resizing

	[self lockOpenGLContext];

	NSRect rect = [self bounds];
	cocos2d::CCEGLView *eglView = cocos2d::CCEGLView::sharedOpenGLView();
	if (eglView)
	{
		eglView->handleScreenResize(rect.size.width, rect.size.height);
	}

	cocos2d::CCDirector *director = cocos2d::CCDirector::sharedDirector();
//	CGSize size = NSSizeToCGSize(rect.size);
//	cocos2d::CCSize ccsize = cocos2d::CCSizeMake(size.width, size.height);
	//director->reshapeProjection(ccsize);
	
	// avoid flicker
	director->drawScene();
//	[self setNeedsDisplay:YES];
	
	[self unlockOpenGLContext];
    
    [super reshape];
}

-(void) lockOpenGLContext
{
	NSOpenGLContext *glContext = [self openGLContext];
	NSAssert( glContext, @"FATAL: could not get openGL context");

	[glContext makeCurrentContext];
	CGLLockContext((CGLContextObj)[glContext CGLContextObj]);
}

-(void) unlockOpenGLContext
{
	NSOpenGLContext *glContext = [self openGLContext];
	NSAssert( glContext, @"FATAL: could not get openGL context");

	CGLUnlockContext((CGLContextObj)[glContext CGLContextObj]);
}

- (void) dealloc
{
	CCLOGINFO(@"cocos2d: deallocing EAGLView %@", self);
	[super dealloc];
}
	
-(int) getWidth
{
	NSSize bound = [self bounds].size;
	return bound.width;
}

-(int) getHeight
{
	NSSize bound = [self bounds].size;
	return bound.height;
}

-(void) swapBuffers
{
#if CC_USE_IMGUI > 0
    [self drawImGui];
#endif // #if CC_USE_IMGUI > 0
}

//
// setFullScreen code taken from GLFullScreen example by Apple
//
- (void) setFullScreen:(BOOL)fullscreen
{
	// Mac OS X 10.6 and later offer a simplified mechanism to create full-screen contexts
#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5

    if (isFullScreen_ == fullscreen)
		return;

	EAGLView *openGLview = [[self class] sharedEGLView];

    if( fullscreen ) {
        originalWinRect_ = [openGLview frame];

        // Cache normal window and superview of openGLView
        if(!windowGLView_)
            windowGLView_ = [[openGLview window] retain];

        [superViewGLView_ release];
        superViewGLView_ = [[openGLview superview] retain];


        // Cover the screen that hosts the game window (correct monitor + aspect sync).
        NSScreen *screen = [[openGLview window] screen];
        if (!screen)
            screen = [NSScreen mainScreen];
        NSRect displayRect = [screen frame];

        // Create a screen-sized window on the display you want to take over
        fullScreenWindow_ = [[CCWindow alloc] initWithFrame:displayRect fullscreen:YES];

        // Remove glView from window
        [openGLview removeFromSuperview];

        // Set new frame
        [openGLview setFrame:displayRect];

        // Attach glView to fullscreen window
        [fullScreenWindow_ setContentView:openGLview];

        // Show the fullscreen window
        [fullScreenWindow_ makeKeyAndOrderFront:self];
		[fullScreenWindow_ makeMainWindow];
		//[fullScreenWindow_ setNextResponder:superViewGLView_];

        SyncCocosFrameFromNSView(openGLview);

    } else {

        // Remove glView from fullscreen window
        [openGLview removeFromSuperview];

        // Release fullscreen window
        [fullScreenWindow_ release];
        fullScreenWindow_ = nil;

        // Attach glView to superview
        [superViewGLView_ addSubview:openGLview];

        // Set new frame
        [openGLview setFrame:originalWinRect_];

        // Show the window
        [windowGLView_ makeKeyAndOrderFront:self];
		[windowGLView_ makeMainWindow];

        SyncCocosFrameFromNSView(openGLview);
    }
	
	// issue #1189
	if (fullscreen)
		[fullScreenWindow_ makeFirstResponder:openGLview];
	else
		[windowGLView_ makeFirstResponder:openGLview];

    isFullScreen_ = fullscreen;

    //[openGLview retain]; // Retain +1

	// is this necessary?
    // re-configure glView
	//cocos2d::CCDirector *director = cocos2d::CCDirector::sharedDirector();
	//director->setOpenGLView(openGLview); //[self setView:openGLview];

    //[openGLview release]; // Retain -1

    [openGLview setNeedsDisplay:YES];
#else
#error Full screen is not supported for Mac OS 10.5 or older yet
#error If you don't want FullScreen support, you can safely remove these 2 lines
#endif
}

#if CC_DIRECTOR_MAC_USE_DISPLAY_LINK_THREAD
#define DISPATCH_EVENT(__event__, __selector__) [eventDelegate_ queueEvent:__event__ selector:__selector__];
#else
#define DISPATCH_EVENT(__event__, __selector__)												\
id obj = eventDelegate_;																		\
[obj performSelector:__selector__															\
		onThread:[(cocos2d::CCDirector*)[CCDirector sharedDirector] runningThread]			\
	  withObject:__event__																	\
   waitUntilDone:NO];
#endif

#pragma mark EAGLView - Mouse events

- (void)mouseDown:(NSEvent *)theEvent
{
    DISPATCH_EVENT_TO_IMGUI(theEvent);
    
    mousePressed = true;
	NSPoint event_location = [theEvent locationInWindow];
	NSPoint local_point = [self convertPoint:event_location fromView:nil];
	
	float x = local_point.x;
	float y = [self getHeight] - local_point.y;
	
    int ids[1] = {0};
    float xs[1] = {0.0f};
    float ys[1] = {0.0f};
    
	ids[0] = [theEvent eventNumber];
	xs[0] = x / frameZoomFactor_;
	ys[0] = y / frameZoomFactor_;

	cocos2d::CCDirector::sharedDirector()->getOpenGLView()->handleTouchesBegin(1, ids, xs, ys);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    DISPATCH_EVENT_TO_IMGUI(theEvent);
    
    if (mousePressed) {
        DISPATCH_EVENT(theEvent, _cmd);
    }
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    DISPATCH_EVENT_TO_IMGUI(theEvent);
    
    if (!mousePressed) return;
	NSPoint event_location = [theEvent locationInWindow];
	NSPoint local_point = [self convertPoint:event_location fromView:nil];
	
	float x = local_point.x;
	float y = [self getHeight] - local_point.y;

    int ids[1] = {0};
    float xs[1] = {0.0f};
    float ys[1] = {0.0f};
    
	ids[0] = [theEvent eventNumber];
	xs[0] = x / frameZoomFactor_;
	ys[0] = y / frameZoomFactor_;

	cocos2d::CCDirector::sharedDirector()->getOpenGLView()->handleTouchesMove(1, ids, xs, ys);
}

- (void)mouseUp:(NSEvent *)theEvent
{
    DISPATCH_EVENT_TO_IMGUI(theEvent);
    
    if (mousePressed == false) return ;
    mousePressed = false;
	NSPoint event_location = [theEvent locationInWindow];
	NSPoint local_point = [self convertPoint:event_location fromView:nil];
	
	float x = local_point.x;
	float y = [self getHeight] - local_point.y;

    int ids[1] = {0};
    float xs[1] = {0.0f};
    float ys[1] = {0.0f};
    
	ids[0] = [theEvent eventNumber];
	xs[0] = x / frameZoomFactor_;
	ys[0] = y / frameZoomFactor_;

	cocos2d::CCDirector::sharedDirector()->getOpenGLView()->handleTouchesEnd(1, ids, xs, ys);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);

    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	// pass the event along to the next responder (like your NSWindow subclass)
	[super rightMouseDown:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super rightMouseDragged:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super rightMouseUp:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super otherMouseDown:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super otherMouseDragged:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super otherMouseUp:theEvent];
}

- (void)mouseEntered:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super mouseEntered:theEvent];
}

- (void)mouseExited:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super mouseExited:theEvent];
}

-(void) scrollWheel:(NSEvent *)theEvent {
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	[super scrollWheel:theEvent];
}

#pragma mark EAGLView - Key events

-(BOOL) becomeFirstResponder
{
	return YES;
}

-(BOOL) acceptsFirstResponder
{
	return YES;
}

-(BOOL) resignFirstResponder
{
	return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
	
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	// pass the event along to the next responder (like your NSWindow subclass)
	[super keyDown:theEvent];
}

- (void)keyUp:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);

    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
    
	// pass the event along to the next responder (like your NSWindow subclass)
	[super keyUp:theEvent];
}

- (void)flagsChanged:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
    
    #if CC_USE_IMGUI > 0
        ImGui_ImplOSX_HandleEvent(theEvent, self);
    #endif // #if CC_USE_IMGUI > 0
}

#pragma mark EAGLView - Touch events
- (void)touchesBeganWithEvent:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
}

- (void)touchesMovedWithEvent:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
}

- (void)touchesEndedWithEvent:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
}

- (void)touchesCancelledWithEvent:(NSEvent *)theEvent
{
	DISPATCH_EVENT(theEvent, _cmd);
}

- (float) getFrameZoomFactor
{
    return frameZoomFactor_;
}

-(BOOL)acceptsFirstMouse:(NSEvent *)event
{
    return YES;
}

#pragma mark - ImGui -

#if CC_USE_IMGUI > 0
-(void) setupImGui
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplOSX_Init();
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
}

-(void) prepareImGui
{
   // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplOSX_NewFrame(self);
    ImGui::NewFrame();
}

-(void) drawDemo
{
    // Global data for the demo
    static bool show_demo_window = true;
    static bool show_another_window = false;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f


        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

-(void) drawImGui
{
    [self prepareImGui];
    [self drawDemo];
    
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glUseProgram(0);
    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplOpenGL2_RenderDrawData(draw_data);
    glUseProgram(last_program);
}
#endif // #if CC_USE_IMGUI > 0

#pragma mark -

@end
