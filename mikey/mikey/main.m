//
//  main.m
//  mikey
//
//  Created by Jon on 10/23/22.
//

#import "AppDelegate.h"

AppDelegate* delegate;

int main(int argc, const char * argv[]) {
	@autoreleasepool {
	    // Setup code that might create autoreleased objects goes here.
		delegate = [[AppDelegate alloc] init];
		[[NSApplication sharedApplication] setDelegate:delegate];
	}
	return NSApplicationMain(argc, argv);
}
