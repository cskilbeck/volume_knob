//
//  AppDelegate.m
//  mikey
//
//  Created by Jon on 10/23/22.
//

#import "AppDelegate.h"
#import <AVFoundation/AVFoundation.h>



@interface OverlayImageView : NSImageView
@end



NSStatusItem* statusItem;
NSWindow* overlayWin;
NSWindow* aboutWin;
AudioObjectID* deviceList = NULL;
int numDevices = 0;
bool muted = false;
CGFloat overlayAlpha = 0.75;



bool deviceHasBuffersInScope( AudioObjectID deviceID, AudioObjectPropertyScope scope )
{
	AudioObjectPropertyAddress propertyAddress = {
		.mSelector  = kAudioDevicePropertyStreamConfiguration,
		.mScope     = scope,
		.mElement   = kAudioObjectPropertyElementWildcard
	};

	UInt32 dataSize = 0;
	OSStatus result = AudioObjectGetPropertyDataSize( deviceID, &propertyAddress, 0, NULL, &dataSize );
	if( result != kAudioHardwareNoError ) {
		return false;
	}

	AudioBufferList *bufferList = malloc( dataSize );
	if( !bufferList ) {
		return false;
	}

	result = AudioObjectGetPropertyData( deviceID, &propertyAddress, 0, NULL, &dataSize, bufferList );
	if( result != kAudioHardwareNoError ) {
		free( bufferList );
		return false;
	}

	bool supportsScope = ( bufferList->mNumberBuffers > 0 );
	free( bufferList );

	return supportsScope;
}



void getInputDevices(void)
{
	numDevices = 0;
	if( deviceList ) {
		free( deviceList );
		deviceList = NULL;
	}

	AudioObjectPropertyAddress propertyAddress = {
		.mSelector  = kAudioHardwarePropertyDevices,
		.mScope     = kAudioObjectPropertyScopeGlobal,
		.mElement   = kAudioObjectPropertyElementWildcard
	};

	UInt32 dataSize = 0;
	OSStatus result = AudioObjectGetPropertyDataSize( kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize );
	if( result != kAudioHardwareNoError ) {
		return;
	}

	deviceList = (AudioObjectID*)malloc( dataSize );

	result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, deviceList );
	if( kAudioHardwareNoError != result ) {
		return;
	}

	for( int i = 0; i < dataSize / sizeof(AudioObjectID); i++ ) {
		if( deviceHasBuffersInScope( deviceList[ i ], kAudioObjectPropertyScopeInput )) {
			deviceList[ numDevices++ ] = deviceList[ i ];
		}
	}
}



NSImage* getMicImage(void) {
	NSImage* image;
	NSImageSymbolConfiguration* symConfig;
	if( muted ) {
		image = [NSImage imageWithSystemSymbolName:@"mic.slash.fill" accessibilityDescription:nil];
		symConfig = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemRedColor, NSColor.systemGrayColor]];
	} else {
		image = [NSImage imageWithSystemSymbolName:@"mic.fill" accessibilityDescription:nil];
		symConfig = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.greenColor]];
	}
	return [image imageWithSymbolConfiguration:symConfig];
}



NSImage* getBigMicImage(void) {
	NSImage* image;
	NSImageSymbolConfiguration* symConfig;
	if( muted ) {
		image = [NSImage imageWithSystemSymbolName:@"mic.slash.circle.fill" accessibilityDescription:nil];
		symConfig = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemRedColor, [NSColor colorWithRed:0.2 green:0.0 blue:0.0 alpha:1.0]]];
	} else {
		image = [NSImage imageWithSystemSymbolName:@"mic.circle.fill" accessibilityDescription:nil];
		symConfig = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemGreenColor, [NSColor colorWithRed:0.0 green:0.5 blue:0.0 alpha:1.0]]];
	}
	return [image imageWithSymbolConfiguration:symConfig];
}



void setMicon(void) {
	statusItem.button.image = getMicImage();
	NSShadow* shadow = nil;
	if( !muted ) {
		NSShadow* shadow = [[NSShadow alloc] init];
		[shadow setShadowColor:NSColor.blackColor];
		[shadow setShadowBlurRadius:2.0];
	}
	statusItem.button.shadow = shadow;
}



void showOverlay(void) {

	if( !overlayWin ) {
		overlayWin = [[NSWindow alloc] init];
		[overlayWin setStyleMask:NSWindowStyleMaskBorderless];
		[overlayWin setOpaque:NO];
		[overlayWin setLevel:kCGDockWindowLevel];
		[overlayWin setBackgroundColor:NSColor.clearColor];
		[overlayWin setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces];
		[overlayWin setFrameAutosaveName: @"win"];
	}
	[overlayWin setAlphaValue:overlayAlpha];

	CGFloat screenWidth = [[overlayWin screen] frame].size.width;
	CGFloat overlayHeight = round( screenWidth * .05 );

	NSImage* image = getBigMicImage();
	NSSize imageSize = [image size];
	CGFloat aspect = imageSize.width / imageSize.height;
	NSRect rect = NSMakeRect( 0.0, 0.0, overlayHeight * aspect, overlayHeight );
	image = [image initWithCGImage:[image CGImageForProposedRect:&rect context:nil hints:nil] size:rect.size];

	OverlayImageView* iv = [OverlayImageView imageViewWithImage:image];

	NSShadow* shadow = [[NSShadow alloc] init];
	//[shadow setShadowColor:NSColor.blackColor];
	[shadow setShadowBlurRadius:4.0];
	[iv setShadow:shadow];

	[overlayWin setContentView:iv];

	[overlayWin makeKeyAndOrderFront:nil];
}



void muteToggle( bool forceUnmute ) {

	getInputDevices();

	UInt32 channel = 0;
	AudioObjectPropertyAddress propVolume = {
		kAudioDevicePropertyVolumeScalar,
		kAudioDevicePropertyScopeInput,
		channel
	};

	AudioObjectPropertyAddress propMute = {
		kAudioDevicePropertyMute,
		kAudioDevicePropertyScopeInput,
		channel
	};

	AudioObjectPropertyAddress propName = {
		kAudioDevicePropertyDeviceNameCFString,
		kAudioDevicePropertyScopeInput,
		channel
	};

	bool anyNotMuted = false;
	UInt32 mute;
	UInt32 dataSize = sizeof( mute );

	for( int i = 0; i < numDevices; i++ ) {

		AudioObjectID dev = deviceList[ i ];

		//NSLog( @"device %d\n", dev );

		OSStatus result;

		CFStringRef name;
		UInt32 sizeOfPtr = 8;

		result = AudioObjectGetPropertyData( dev, &propName, 0, NULL, &sizeOfPtr, &name);

		if( kAudioHardwareNoError != result ) {
			NSLog( @"ERROR\n" );
			continue;
		}

		NSLog( @"device «%@»\n", (__bridge NSString*)name);


		if( !AudioObjectHasProperty( dev, &propVolume ) ) {
			NSLog( @"NO VOLUME\n" );
			//continue;
		}
		if( !AudioObjectHasProperty( dev, &propMute) ) {
			NSLog( @"NO MUTE\n" );
			continue;
		}

		result = AudioObjectGetPropertyData( dev, &propMute, 0, NULL, &dataSize, &mute );

		if( kAudioHardwareNoError != result ) {
			NSLog( @"ERROR\n" );
			continue;
		}

		//NSLog( @"current mute = %d\n", mute );

		if( mute == 0 ) {
			anyNotMuted = true;
		}
	}

	muted = ( anyNotMuted && !forceUnmute ? 1 : 0 );
	mute = muted;

	for( int i = 0; i < numDevices; i++ ) {

		AudioObjectID dev = deviceList[ i ];

		OSStatus result;
		result = AudioObjectSetPropertyData( dev, &propMute, 0, NULL, dataSize, &mute );

		if( kAudioHardwareNoError != result ) {
			NSLog( @"ERROR\n" );
			continue;
		}
	}

	setMicon();
	if( overlayWin ) {
		showOverlay();
	}

}



CGEventRef __nullable keyHookCallback( CGEventTapProxy proxy, CGEventType type, CGEventRef cgevent, void * __nullable userInfo ) {
	NSEvent* event = [NSEvent eventWithCGEvent:cgevent];
	NSString* chars = [event characters];
	if( [chars length] == 1 ) {
		int ch0 = [event.characters characterAtIndex:0];
		if( ch0 == NSF10FunctionKey ) {
			//NSLog( @"BINGO!\n" );
			muteToggle( false );
			return nil;
		} else {
			//NSLog( @"key = 0x%04x\n", ch0 );
		}
	}
	return cgevent;
}



@implementation AppDelegate



- (void)windowWillClose:(NSNotification *)notification {
	if( aboutWin == [notification object] ) {
		aboutWin = nil;
	}
}



- (void)menuAbout {
	CGFloat fontSize = 60.0;
	CGFloat w = fontSize * 7;

	if( !aboutWin || ![aboutWin isOnActiveSpace] ) {
		if( aboutWin ) {
			[aboutWin close];
		}
		aboutWin = [[NSWindow alloc] init];
		[aboutWin setTitle:@"About Mikey"];
		[aboutWin setReleasedWhenClosed:false];
		[aboutWin setStyleMask:NSWindowStyleMaskClosable | NSWindowStyleMaskTitled];
		[aboutWin setBackgroundColor:NSColor.systemMintColor];
		NSArray<NSTextField*>* textFields = @[
			[NSTextField labelWithString:@"BUY"],
			[NSTextField labelWithString:@"THE"],
			[NSTextField labelWithString:@"THING!"],
		];
		NSFont* font = [NSFont boldSystemFontOfSize:fontSize];
		for( NSTextField* field in textFields ) {
			[field setTextColor:NSColor.systemYellowColor];
			[field setFont:font];
		}

		NSStackView* view = [NSStackView stackViewWithViews:textFields];
		[view setOrientation:NSUserInterfaceLayoutOrientationVertical];
		[view setDistribution:NSStackViewDistributionFill];
		[view setEdgeInsets:NSEdgeInsetsMake( fontSize, 0, fontSize, 0 )];
		[aboutWin setContentView: view];
		[aboutWin setDelegate:self];
	}
	CGSize screenSize = [[aboutWin screen] frame].size;
	for( int i = 0; i < 2; i++ ) {
		CGFloat h = [aboutWin frame].size.height;
		NSRect rect = NSMakeRect( ( screenSize.width - w ) * 0.5, ( screenSize.height - h ) * 0.5, w, h );
		[aboutWin setFrame:rect display:true];
		[aboutWin makeKeyAndOrderFront:nil];
	}
	[aboutWin orderFrontRegardless];
}



-(void)toggleMute {
	muteToggle( false );
}



-(void)opacitySliderAction:(NSSlider*)slider {
	overlayAlpha = [slider doubleValue];
	[overlayWin setAlphaValue:overlayAlpha];
}



- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

	[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

	CFMachPortRef tap = CGEventTapCreate( kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, CGEventMaskBit( kCGEventKeyDown ), keyHookCallback, NULL );
	CFRunLoopSourceRef rls = CFMachPortCreateRunLoopSource( kCFAllocatorDefault, tap, 0 );
	CFRunLoopAddSource( CFRunLoopGetMain(), rls, kCFRunLoopCommonModes );

	NSMenu* statusMenu = [[NSMenu alloc] initWithTitle:@"menu"];

	{
		[statusMenu addItemWithTitle:@"Overlay opacity:" action:nil keyEquivalent:@""];
		NSSlider* slider = [NSSlider sliderWithValue:overlayAlpha minValue:0.0 maxValue:1.0 target:self action:@selector(opacitySliderAction:)];
		[slider setControlSize:NSControlSizeSmall];
		NSMenuItem* sliderItem = [[NSMenuItem alloc] init];
		CGFloat sliderInset = 14.0;
		NSView* sliderView = [[NSView alloc] initWithFrame:NSMakeRect( 0.0, 0.0, slider.frame.size.width + sliderInset * 2.0, slider.frame.size.height )];
		[sliderItem setView:sliderView];
		[statusMenu addItem:sliderItem];
		[sliderView addSubview:slider];
		[slider setFrameOrigin:NSMakePoint( sliderInset, 0.0 )];
	}
	[statusMenu addItem:[NSMenuItem separatorItem]];
	[statusMenu addItemWithTitle:@"Toggle mute" action:@selector(toggleMute) keyEquivalent:@""];
	[statusMenu addItemWithTitle:@"About Mikey" action:@selector(menuAbout) keyEquivalent:@""];
	[statusMenu addItemWithTitle:@"Quit Mikey" action:@selector(terminate:) keyEquivalent:@""];

	statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
	setMicon();

	statusItem.menu = statusMenu;

	muteToggle( true );

	showOverlay();

	//dispatch_after( 1.0, dispatch_get_main_queue(), ^{
	//	[self menuAbout];
	//});
}



- (void)applicationWillTerminate:(NSNotification *)aNotification {
	muteToggle( true );
}



- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
	return YES;
}



@end



@implementation OverlayImageView
- (void)mouseDown:(NSEvent *)event {
	if( event.clickCount == 2 ) {
		muteToggle( false );
	} else {
		[[self window] performWindowDragWithEvent:event];
	}
}
@end



