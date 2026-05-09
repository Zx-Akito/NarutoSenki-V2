/****************************************************************************
 Copyright (c) 2010 cocos2d-x.org
 
 http://www.cocos2d-x.org
 ****************************************************************************/

#import "AppController.h"
#import "cocos2d.h"
#import "EAGLView.h"
#import "AppDelegate.h"
#import <Foundation/Foundation.h>

typedef void (*MacWsEventCallback)(const char *eventName, const char *payload);
static MacWsEventCallback s_macWsCallback = nullptr;

@interface MacWsClient : NSObject <NSURLSessionWebSocketDelegate>
{
	NSURLSession *_session;
	NSURLSessionWebSocketTask *_task;
	BOOL _connected;
}
+ (instancetype)shared;
- (BOOL)connectToUrlString:(NSString *)urlString;
- (void)sendText:(NSString *)text;
- (void)disconnect;
- (BOOL)isConnected;
@end

@implementation MacWsClient

+ (instancetype)shared
{
	static MacWsClient *instance = nil;
	if (!instance)
	{
		instance = [[MacWsClient alloc] init];
	}
	return instance;
}

- (void)dealloc
{
	[self disconnect];
	[super dealloc];
}

- (void)emitEvent:(const char *)eventName payload:(NSString *)payload
{
	if (!s_macWsCallback)
		return;
	const char *payloadCStr = payload ? [payload UTF8String] : "";
	s_macWsCallback(eventName, payloadCStr ? payloadCStr : "");
}

- (BOOL)connectToUrlString:(NSString *)urlString
{
	[self disconnect];
	NSURL *url = [NSURL URLWithString:urlString];
	if (!url)
		return NO;

	NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
	_session = [[NSURLSession sessionWithConfiguration:config
											  delegate:self
										 delegateQueue:[NSOperationQueue mainQueue]] retain];
	_task = [[_session webSocketTaskWithURL:url] retain];
	[_task resume];
	[self receiveLoop];
	return YES;
}

- (void)receiveLoop
{
	if (!_task)
		return;

	[_task receiveMessageWithCompletionHandler:^(NSURLSessionWebSocketMessage *_Nullable message, NSError *_Nullable error) {
	 	if (error)
	 	{
	 		_connected = NO;
	 		[self emitEvent:"error" payload:[error localizedDescription]];
	 		return;
	 	}
	 	if (message)
	 	{
	 		if (message.type == NSURLSessionWebSocketMessageTypeString)
	 		{
	 			[self emitEvent:"message" payload:message.string ?: @""];
	 		}
	 		else if (message.type == NSURLSessionWebSocketMessageTypeData)
	 		{
	 			[self emitEvent:"message" payload:@"[binary]"];
	 		}
	 	}
	 	[self receiveLoop];
	 }];
}

- (void)sendText:(NSString *)text
{
	if (!_task || !_connected)
		return;
	NSURLSessionWebSocketMessage *message = [[[NSURLSessionWebSocketMessage alloc] initWithString:text ?: @""] autorelease];
	[_task sendMessage:message completionHandler:^(NSError *_Nullable error) {
	 	if (error)
	 	{
	 		[self emitEvent:"error" payload:[error localizedDescription]];
	 	}
	 }];
}

- (void)disconnect
{
	if (_task)
	{
		[_task cancelWithCloseCode:NSURLSessionWebSocketCloseCodeNormalClosure reason:nil];
		[_task release];
		_task = nil;
	}
	if (_session)
	{
		[_session invalidateAndCancel];
		[_session release];
		_session = nil;
	}
	_connected = NO;
}

- (BOOL)isConnected
{
	return _connected;
}

- (void)URLSession:(NSURLSession *)session
	webSocketTask:(NSURLSessionWebSocketTask *)webSocketTask
didOpenWithProtocol:(NSString *)protocol
{
	_connected = YES;
	[self emitEvent:"open" payload:@""];
}

- (void)URLSession:(NSURLSession *)session
	webSocketTask:(NSURLSessionWebSocketTask *)webSocketTask
didCloseWithCode:(NSURLSessionWebSocketCloseCode)closeCode
		  reason:(NSData *)reason
{
	_connected = NO;
	[self emitEvent:"close" payload:@""];
}

@end

extern "C" void MacWsSetEventCallback(void (*callback)(const char *eventName, const char *payload))
{
	s_macWsCallback = callback;
}

extern "C" bool MacWsConnect(const char *url)
{
	NSString *urlString = [NSString stringWithUTF8String:(url ? url : "")];
	return [[MacWsClient shared] connectToUrlString:urlString] ? true : false;
}

extern "C" void MacWsSend(const char *message)
{
	NSString *text = [NSString stringWithUTF8String:(message ? message : "")];
	[[MacWsClient shared] sendText:text];
}

extern "C" void MacWsDisconnect()
{
	[[MacWsClient shared] disconnect];
}

extern "C" bool MacWsIsConnected()
{
	return [[MacWsClient shared] isConnected] ? true : false;
}

@implementation AppController

@synthesize window;

static AppDelegate s_sharedApplication;

-(void) applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Smaller default desktop window for local multi-instance testing.
	NSRect rect = NSMakeRect(200, 200, (CGFloat)960, (CGFloat)540);
	NSUInteger windowStyle = NSClosableWindowMask | NSMiniaturizableWindowMask |
		NSResizableWindowMask | NSTitledWindowMask;

	window = [[NSWindow alloc]
		initWithContentRect:rect
		styleMask:windowStyle
		backing:NSBackingStoreBuffered
		defer:YES];

	EAGLView *glView = [[EAGLView alloc]
		initWithFrame:NSMakeRect(0.0f, 0.0f, rect.size.width, rect.size.height)];

	[window setContentView:glView];
	[window becomeFirstResponder];
	[window setTitle:@"Naruto Senki"];
	[window makeKeyAndOrderFront:self];
	[window center];
	[window setAcceptsMouseMovedEvents:YES];
	// Keep GL content aspect ratio when resizing (drag corners); avoids stretched/squashed output.
	if (rect.size.width > 0.0 && rect.size.height > 0.0)
	{
		[window setContentAspectRatio:NSMakeSize(rect.size.width, rect.size.height)];
	}

	// Must run GL view init before Lua / director access in AppDelegate.
	cocos2d::CCApplication::sharedApplication()->run();
}

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

-(void) dealloc
{
	Director::sharedDirector()->end();
	[super dealloc];
}

#pragma mark -

-(IBAction) toggleFullScreen:(id)sender
{
	EAGLView *pView = [EAGLView sharedEGLView];
	[pView setFullScreen:!pView.isFullScreen];
}

-(IBAction) exitFullScreen:(id)sender
{
	[[EAGLView sharedEGLView] setFullScreen:NO];
}

@end
