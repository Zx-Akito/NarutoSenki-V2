#import "RootViewController.h"


@implementation RootViewController

/*The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {Custom initialization
    }
    return self;
}
*/

/*Implement loadView to create a view hierarchy programmatically, without using a nib.
- ()loadView {
}
*/

/*Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- ()viewDidLoad {
    [super viewDidLoad];
}
 
*/Override to allow orientations other than the default portrait orientation.This method is deprecated on ios6
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return UIInterfaceOrientationIsLandscape( interfaceOrientation );
}For ios6, use supportedInterfaceOrientations & shouldAutorotate instead
- (NSUInteger) supportedInterfaceOrientations{
#ifdef __IPHONE_6_0
    return UIInterfaceOrientationMaskAllButUpsideDown;
#endif
}

- (BOOL) shouldAutorotate {
    return YES;
}

//fix not hide status on ios7
- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- ()didReceiveMemoryWarning {Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];Release any cached data, images, etc that aren't in use.
}

- ()viewDidUnload {
    [super viewDidUnload];Release any retained subviews of the main view.e.g. self.myOutlet = nil;
}


- ()dealloc {
    [super dealloc];
}


@end
