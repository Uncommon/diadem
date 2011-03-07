// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

#include "Diadem/NativeCocoa.h"
#include "Diadem/Factory.h"
#include "Diadem/Layout.h"

// Measurements (mostly for padding) have two main sources: the Apple Human
// Interface Guidelines (AHIG), and the distances that Interface Builder (IB)
// snaps to. Conflicts are resolved by judgment call.

namespace {

NSString* NSStringWithString(const Diadem::String string) {
  return [NSString stringWithCString:string encoding:NSUTF8StringEncoding];
}

class ScopedAutoreleasePool {
 public:
  ScopedAutoreleasePool() : pool_([[NSAutoreleasePool alloc] init]) {}
  ~ScopedAutoreleasePool() { [pool_ drain]; }

  NSAutoreleasePool *pool_;
};

template <ThemeMetric m>
SInt32 GetThemeMetric() {
  static SInt32 result = INT_MAX;

  if (result == INT_MAX)
    ::GetThemeMetric(m, &result);
  return result;
}

} // namespace

@interface WindowDelegate : NSObject {
  Diadem::Cocoa::Window *window_;
}
@end

@implementation WindowDelegate

- (id)initWithNativeWindow:(Diadem::Cocoa::Window*)window {
  if ([super init] == nil)
    return nil;
  window_ = window;
  return self;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
  Diadem::Window *window = window_->GetEntity()->GetWindow();
  DASSERT(window != NULL);

  return window->AttemptClose();
}

@end


@interface ButtonTarget : NSObject {
 @private
  Diadem::Cocoa::Button *button_;
}

- (id)initWithButton:(Diadem::Cocoa::Button*)button;
- (void)click:(id)sender;

@end

@implementation ButtonTarget

- (id)initWithButton:(Diadem::Cocoa::Button*)button {
  if ([super init] == nil)
    return nil;
  button_ = button;
  [(NSButton*)button->GetNativeRef() setTarget:self];
  [(NSButton*)button->GetNativeRef() setAction:@selector(click:)];
  return self;
}

- (void)click:(id)sender {
  button_->GetEntity()->Clicked();
}

@end

// NSPathControl doesn't have a border, so wrap it so that we can draw one
@interface PathBoxControl : NSView {
 @private
  NSPathControl *pathControl_;
  NSString *path_;
}

- (void)setPath:(NSString*)path;
- (NSString*)path;

@end

@implementation PathBoxControl

- (id)initWithFrame:(NSRect)frame {
  if ([super initWithFrame:frame] == nil)
    return nil;

  NSRect sub_rect = NSInsetRect(frame, 1, 1);

  sub_rect.origin = NSMakePoint(1, 1);
  pathControl_ = [[NSPathControl alloc] initWithFrame:sub_rect];
  [pathControl_ setAutoresizingMask:
      NSViewWidthSizable | NSViewHeightSizable |
      NSViewMinXMargin | NSViewMaxYMargin];
  [pathControl_ setBackgroundColor:[NSColor whiteColor]];
  [self addSubview:pathControl_];
  return self;
}

- (void)dealloc {
  [path_ release];
  [super dealloc];
}

// This is the correct value, even though there seems to be no standard
// constant for it.
#define kFolderFileType 'fldr'

- (void)setPath:(NSString*)path {
  // Store a copy of the path because if it doesn't exist on disk, the URL in
  // the path control will only be the portion that does exist.
  [path_ autorelease];
  path_ = [path copy];

  if ([[NSFileManager defaultManager] fileExistsAtPath:path_]) {
    [pathControl_ setURL:[NSURL fileURLWithPath:path_]];
  } else {
    // Find a parent folder that exists, and put in fake entries for the
    // non-existent children, assuming they're supposed to be folders
    NSMutableArray *fake_paths = [NSMutableArray array];
    NSString *parent = [[path_ copy] autorelease];

    while (![parent isEqualToString:@"/"]) {
      [fake_paths insertObject:parent atIndex:0];
      parent = [parent stringByDeletingLastPathComponent];
      if ([[NSFileManager defaultManager] fileExistsAtPath:parent]) {
        [pathControl_ setURL:[NSURL fileURLWithPath:parent]];

        NSMutableArray *cells =
            [[pathControl_ pathComponentCells] mutableCopy];
        NSImage *folder_icon = [[NSWorkspace sharedWorkspace]
              iconForFileType:NSFileTypeForHFSTypeCode(kFolderFileType)];

        for (NSUInteger i = 0; i < [fake_paths count]; ++i) {
          NSPathComponentCell *cell =
              [[[NSPathComponentCell alloc] init] autorelease];

          // TODO(catmull): use a document icon for the last cell if the path
          // doesn't end in a slash
          [cell setStringValue:
              [[fake_paths objectAtIndex:i] lastPathComponent]];
          [cell setURL:[NSURL fileURLWithPath:[fake_paths objectAtIndex:i]]];
          [cell setImage:folder_icon];
          [cells addObject:cell];
        }
        [pathControl_ setPathComponentCells:cells];
        return;
      }
    }
    // Nothing in the path exists
    [pathControl_ setURL:[NSURL URLWithString:@""]];
  }
}

- (NSString*)path {
  return path_;
}

- (void)drawRect:(NSRect)frame {
  const HIThemeFrameDrawInfo info = { 0,
      kHIThemeFrameTextFieldSquare,
      kThemeStateActive,
      false };

  HIThemeDrawFrame(
      (const CGRect*)&frame, &info,
      (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort],
      kHIThemeOrientationInverted);
  [super drawRect:frame];
}

@end


namespace Diadem {

PlatformMetrics Cocoa::NativeCocoa::metrics_ = {
    14, 17, 18,
    Spacing(12, 6, 12, 6) };

void Cocoa::SetUpFactory(Factory *factory) {
  DASSERT(factory != NULL);
  factory->RegisterNative<Label>(kTypeNameLabel);
  factory->RegisterNative<Link>(kTypeNameLink);
  factory->RegisterNative<Window>(kTypeNameWindow);
  factory->RegisterNative<Box>(kTypeNameBox);
  factory->RegisterNative<PushButton>(kTypeNameButton);
  factory->RegisterNative<Checkbox>(kTypeNameCheck);
  factory->RegisterNative<EditField>(kTypeNameEdit);
  factory->RegisterNative<PasswordField>(kTypeNamePassword);
  factory->RegisterNative<PathBox>(kTypeNamePath);
  factory->RegisterNative<Separator>(kTypeNameSeparator);
  factory->RegisterNative<Image>(kTypeNameImage);
  factory->RegisterNative<Popup>(kTypeNamePopup);
  factory->RegisterNative<PopupItem>(kTypeNameItem);
}

String Cocoa::ChooseFolder(const String &initial_path) {
  ScopedAutoreleasePool pool;
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  NSString *initial = strlen(initial_path.Get()) == 0 ? nil :
      NSStringWithString(initial_path);

  [panel setCanChooseFiles:NO];
  [panel setCanChooseDirectories:YES];
  [panel setAllowsMultipleSelection:NO];
  [panel runModalForDirectory:initial file:nil types:nil];

  NSArray *urls = [panel URLs];

  if ([urls count] == 0)
    return String();
  return String([[[urls objectAtIndex:0] path] fileSystemRepresentation]);
}

String Cocoa::ChooseNewPath(
    const String &prompt,
    const String &initial_path,
    const String &initial_name) {
  ScopedAutoreleasePool pool;
  NSSavePanel *panel = [NSSavePanel savePanel];
  NSString *path = nil, *name = nil;

  if (!prompt.IsEmpty())
    [panel setPrompt:[NSString stringWithUTF8String:prompt.Get()]];
  if (!initial_path.IsEmpty())
    path = [NSString stringWithUTF8String:initial_path.Get()];
  if (!initial_name.IsEmpty())
    name = [NSString stringWithUTF8String:initial_name.Get()];

  const NSInteger result = [panel runModalForDirectory:path file:name];

  if (result == NSOKButton)
    return String([[[panel URL] path] UTF8String]);
  else
    return String();
}

NSAlert* Cocoa::AlertForMessageData(MessageData *message) {
  DASSERT(message != NULL);
  NSAlert *alert =
      [NSAlert alertWithMessageText:NSStringWithString(message->message_)
                      defaultButton:nil
                    alternateButton:nil
                        otherButton:nil
          informativeTextWithFormat:@""];

  if (alert == nil)
    return nil;

  [alert setShowsSuppressionButton:message->show_suppress_];
  if (!message->suppress_text_.IsEmpty())
    [[alert suppressionButton] setTitle:
        NSStringWithString(message->suppress_text_)];
  if (!message->accept_text_.IsEmpty())
    [[[alert buttons] objectAtIndex:0] setStringValue:
        NSStringWithString(message->accept_text_)];
  if (message->show_cancel_) {
    NSString *cancel_text;

    if (message->cancel_text_.IsEmpty())
      cancel_text = NSLocalizedString(@"Cancel", "");
    else
      cancel_text = NSStringWithString(message->cancel_text_);
    [alert addButtonWithTitle:cancel_text];
  }
  if (message->show_other_) {
    NSString *other_text;

    if (message->other_text_.IsEmpty())
      other_text = NSLocalizedString(@"Don't Save", "");
    else
      other_text = NSStringWithString(message->other_text_);
    [alert addButtonWithTitle:other_text];
  }
  // TODO(catmull): implement default button setting
  return alert;
}

ButtonType Cocoa::ShowMessage(MessageData *message) {
  ScopedAutoreleasePool pool;
  NSAlert *alert = AlertForMessageData(message);

  if (alert == nil)
    return kCancelButton;

  const NSInteger result = [alert runModal];

  if (message->show_suppress_)
    message->suppressed_ = [[alert suppressionButton] state] == NSOnState;
  switch (result) {
    case NSAlertFirstButtonReturn:
    case NSAlertDefaultReturn:
      return kAcceptButton;
    case NSAlertThirdButtonReturn:
    case NSAlertAlternateReturn:
      return kOtherButton;
    case NSAlertSecondButtonReturn:
    case NSAlertOtherReturn:
    default:
      return kCancelButton;
  }
}

Cocoa::Window::Window()
    : window_ref_(nil), delegate_(nil) {
}

void Cocoa::Window::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  NSUInteger style_mask = NSTitledWindowMask;

  if (properties.Exists(kPropStyle)) {
    const uint32_t style =
        Native::ParseWindowStyle(properties[kPropStyle].Coerce<String>());

    if (style & kStyleClosable)
      style_mask |= NSClosableWindowMask;
    if (style & kStyleResizable)
      style_mask |= NSResizableWindowMask;
    if (style & kStyleMinimizable)
      style_mask |= NSMiniaturizableWindowMask;
  }

  window_ref_ = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 50, 50)
                styleMask:style_mask
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window_ref_ setReleasedWhenClosed:NO];
  [window_ref_ setContentMinSize:NSMakeSize(10, 10)];
  [window_ref_ center];
  delegate_ = [[WindowDelegate alloc] initWithNativeWindow:this];
  [window_ref_ setDelegate:delegate_];
  if ((window_ref_ != nil) && properties.Exists(kPropText)) {
    const String title_string = properties[kPropText].Coerce<String>();

    [window_ref_ setTitle:NSStringWithString(title_string)];
  }
}

Cocoa::Window::~Window() {
  ScopedAutoreleasePool pool;

  Close();
  [window_ref_ release];
  [delegate_ release];
}

bool Cocoa::Window::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (window_ref_ == nil)
    return false;
  if (strcmp(name, kPropText) == 0) {
    [window_ref_ setTitle:NSStringWithString(value.Coerce<String>())];
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>();

    [window_ref_ setContentSize:NSMakeSize(size.width, size.height)];
    // TODO: reposition?
    return true;
  }
  if (strcmp(name, kPropLocation) == 0) {
    const NSRect screen_frame = [[window_ref_ screen] frame];
    const Location location = value.Coerce<Location>();

    [window_ref_ setFrameTopLeftPoint:NSMakePoint(location.x,
        screen_frame.size.height - location.y)];
  }
  return false;
}

Value Cocoa::Window::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (window_ref_ == nil)
    return false;
  if (strcmp(name, kPropText) == 0) {
    NSString *title = [window_ref_ title];

    return String([title UTF8String]);
  }
  if (strcmp(name, kPropSize) == 0) {
    NSRect content_rect =
        [window_ref_ contentRectForFrameRect:[window_ref_ frame]];

    return Size(content_rect.size.width, content_rect.size.height);
  }
  if (strcmp(name, kPropMargins) == 0) {
    return Value(Spacing(14, 20, 20, 20));
  }
  if (strcmp(name, kPropVisible) == 0) {
    return Value((bool)[window_ref_ isVisible]);
  }
  return Value();
}

void Cocoa::Window::AddChild(Native *child) {
  ScopedAutoreleasePool pool;

  if (![(id)child->GetNativeRef() isKindOfClass:[NSView class]])
    return;

  [[window_ref_ contentView] addSubview:(NSView*)child->GetNativeRef()];
}

bool Cocoa::Window::ShowModeless() {
  ScopedAutoreleasePool pool;

  [window_ref_ makeKeyAndOrderFront:nil];
  return true;
}

bool Cocoa::Window::Close() {
  ScopedAutoreleasePool pool;

  if ([NSApp modalWindow] == window_ref_)
    [NSApp abortModal];
  [window_ref_ orderOut:nil];
  return true;
}

bool Cocoa::Window::ShowModal(void*) {
  ScopedAutoreleasePool pool;

  [NSApp runModalForWindow:window_ref_];
  return true;
}

bool Cocoa::Window::EndModal() {
  ScopedAutoreleasePool pool;

  if ([NSApp modalWindow] != window_ref_)
    return false;
  [NSApp stopModal];
  return true;
}

bool Cocoa::Window::SetFocus(Entity *new_focus) {
  if ((new_focus->GetNative() == NULL) ||
      (new_focus->GetNative()->GetNativeRef() == NULL))
    return false;

  id focus = (id)new_focus->GetNative()->GetNativeRef();

  if (![focus isKindOfClass:[NSResponder class]])
    return false;
  [window_ref_ makeFirstResponder:focus];
  return true;
}

bool Cocoa::Window::TestClose() {
  ScopedAutoreleasePool pool;

  [window_ref_ performClose:nil];
  return true;
}

void Cocoa::View::ConfigureView() {
  // The view needs to be anchored to the top left of its parent
  [view_ref_ setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
}

Cocoa::View::~View() {
  ScopedAutoreleasePool pool;

  [view_ref_ release];
}

NSPoint Cocoa::View::InvertPoint(NSPoint point) const {
  const NSRect super_bounds = [[view_ref_ superview] bounds];

  return NSMakePoint(point.x, super_bounds.size.height - point.y);
}

bool Cocoa::View::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (view_ref_ == NULL)
    return false;
  if (strcmp(name, kPropLocation) == 0) {
    const Spacing inset = GetInset();
    const Location offset = GetViewOffset();
    const Location set_loc = value.Coerce<Location>();
    const Location loc = set_loc + offset + Location(inset.left, inset.top);
    NSPoint origin = InvertPoint(NSMakePoint(loc.x, loc.y));

    [[view_ref_ superview] setNeedsDisplayInRect:[view_ref_ frame]];
    origin.y -= [view_ref_ frame].size.height;
    [view_ref_ setFrameOrigin:origin];
    [view_ref_ setNeedsDisplay:YES];
#if 1
    DASSERT(value.Coerce<Location>() == GetLocation());
#endif
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>() - GetInset();
    NSRect frame = [view_ref_ frame];

    [[view_ref_ superview] setNeedsDisplayInRect:frame];
    frame.origin.y += frame.size.height - size.height;
    frame.size.height = size.height;
    frame.size.width = size.width;
    [view_ref_ setFrame:frame];
    [view_ref_ setNeedsDisplay:YES];
    return true;
  }
  if (strcmp(name, kPropVisible) == 0) {
    [view_ref_ setHidden:!value.Coerce<bool>()];
    return true;
  }
  return false;
}

Value Cocoa::View::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (view_ref_ == nil)
    return Value();
  if (strcmp(name, kPropLocation) == 0) {
    return GetLocation();
  }
  if (strcmp(name, kPropSize) == 0) {
    return GetViewSize() + GetInset();
  }
  if (strcmp(name, kPropVisible) == 0) {
    return (bool)![view_ref_ isHidden];
  }
  return Value();
}

Location Cocoa::View::GetLocation() const {
  const NSPoint location = InvertPoint([view_ref_ frame].origin);
  const Spacing inset = GetInset();

  return Location(location.x, location.y - [view_ref_ frame].size.height) -
      GetViewOffset() - Location(inset.left, inset.top);
}

Size Cocoa::View::GetViewSize() const {
  ScopedAutoreleasePool pool;
  const NSRect frame = [view_ref_ frame];

  return Size(frame.size.width, frame.size.height);
}

void Cocoa::Box::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  const NSRect default_bounds = { { 0, 0 }, { 50, 50 } };

  view_ref_ = [[NSBox alloc] initWithFrame:default_bounds];
  [(NSBox*)view_ref_ setTitlePosition:NSNoTitle];
  [(NSBox*)view_ref_ setContentViewMargins:NSMakeSize(0, 0)];
  [(NSBox*)view_ref_ sizeToFit];  // adjust to new margins
  [view_ref_ setFrame:default_bounds];
  ConfigureView();
}

Value Cocoa::Box::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMargins) == 0) {
    // AHIG: 10, 16, 16, 16  IB: 11, 16, 11, 16
    return Spacing(10, 16, 16, 16);
  }
  if (strcmp(name, kPropPadding) == 0) {
    // AHIG: no recommendation  IB: 8, seems too small
    return Spacing(12, 12, 12, 12);
  }
  return View::GetProperty(name);
}

Spacing Cocoa::Box::GetInset() const {
  return Spacing(-2, -3, -4, -3);
}

void Cocoa::Box::AddChild(Native *child) {
  ScopedAutoreleasePool pool;
  NSView *subview = (NSView*)child->GetNativeRef();

  [[(NSBox*)view_ref_ contentView] addSubview:subview];
  child->GetEntity()->SetProperty(kPropLocation, Location());
}

Location Cocoa::Box::GetSubviewAdjustment() const {
  const Spacing inset = GetInset();

  return Location(-inset.left, -inset.top);
}

bool Cocoa::Control::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropText) == 0) {
    ScopedAutoreleasePool pool;

    [(NSControl*)view_ref_ setStringValue:
        NSStringWithString(value.Coerce<String>())];
    return true;
  }
  if (strcmp(name, kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;

    [(NSControl*)view_ref_ setEnabled:value.Coerce<bool>()];
    return true;
  }
  if (strcmp(name, kPropValue) == 0) {
    [(NSControl*)view_ref_ setIntValue:value.Coerce<int32_t>()];
    return true;
  }
  return View::SetProperty(name, value);
}

// -cellSize is documented to return this in some cases
static const NSSize kMaxCellSize = { 10000, 10000 };

Value Cocoa::Control::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropMinimumSize) == 0) {
    const NSSize cell_size = [[(NSControl*)view_ref_ cell] cellSize];

    if (NSEqualSizes(cell_size, NSZeroSize) ||
        NSEqualSizes(cell_size, kMaxCellSize))
      return GetProperty(kPropSize);

    return Size(cell_size.width, cell_size.height) + GetInset();
  }
  if (strcmp(name, kPropText) == 0) {
    NSString *text = [(NSControl*)view_ref_ stringValue];

    return String([text UTF8String]);
  }
  if (strcmp(name, kPropEnabled) == 0) {
    return static_cast<bool>([(NSControl*)view_ref_ isEnabled]);
  }
  if (strcmp(name, kPropValue) == 0) {
    return static_cast<int32_t>([(NSControl*)view_ref_ intValue]);
  }
  return View::GetProperty(name);
}

Cocoa::Button::Button() : target_(nil) {}

Cocoa::Button::~Button() {
  ScopedAutoreleasePool pool;

  [target_ release];
}

void Cocoa::Button::MakeTarget() {
  target_ = [[ButtonTarget alloc] initWithButton:this];
}

void Cocoa::PushButton::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  NSButton *button = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];

  view_ref_ = button;
  [button setBezelStyle:NSRoundedBezelStyle];
  [button setButtonType:NSMomentaryPushInButton];
  [button setImagePosition:NSNoImage];
  [button setBordered:YES];
  if (properties.Exists(kPropText))
    [button setTitle:
        NSStringWithString(properties[kPropText].Coerce<String>())];
  if (properties.Exists(kPropUISize)) {
    const String ui_size = properties[kPropUISize].Coerce<String>();

    if (ui_size == kUISizeSmall) {
      [[button cell] setControlSize:NSSmallControlSize];
      [[button cell] setFont:
          [NSFont systemFontOfSize:
              [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
    }
  }
  MakeTarget();
  ConfigureView();
}

bool Cocoa::PushButton::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropButtonType) == 0) {
    const String type = value.Coerce<String>();

    if (type == "default")
      [(NSButton*)view_ref_ setKeyEquivalent:@"\r"];
    else if (type == "cancel")
      [(NSButton*)view_ref_ setKeyEquivalent:@"\E"];
    return true;
  }

  return Control::SetProperty(name, value);
}

#define kButtonWidthAdjustment 12

Value Cocoa::PushButton::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropMinimumSize) == 0) {
    Size min_size = Control::GetProperty(kPropMinimumSize).Coerce<Size>();

    // cellSize returns 32 instead of 20, so correct it
    if ([[(NSButton*)view_ref_ cell] controlSize] == NSSmallControlSize)
      min_size.height = GetThemeMetric<kThemeMetricSmallPushButtonHeight>();
    else
      min_size.height = GetThemeMetric<kThemeMetricPushButtonHeight>();
    min_size.width += kButtonWidthAdjustment;
    return min_size;
  }
  if (strcmp(name, kPropPadding) == 0) {
    // AHIG and IB agree on 12/10/8
    switch ([[(NSButton*)view_ref_ cell] controlSize]) {
      case NSRegularControlSize:
        return Spacing(12, 12, 12, 12);
      case NSSmallControlSize:
        return Spacing(10, 10, 10, 10);
      case NSMiniControlSize:
        return Spacing(8, 8, 8, 8);
      default:
        return Value();
    }
  }
  if (strcmp(name, kPropText) == 0) {
    NSString *text = [(NSButton*)view_ref_ title];

    return String([text cStringUsingEncoding:NSUTF8StringEncoding]);
  }
  if (strcmp(name, kPropBaseline) == 0) {
    if ([[(NSButton*)view_ref_ cell] controlSize] == NSSmallControlSize)
      return 13;
    else
      return 14;
  }
  return Control::GetProperty(name);
}

Spacing Cocoa::PushButton::GetInset() const {
  // Extra space on botom for the shadow, apparently.
  // The need for the left/right inset is mysterious.
  if ([[(NSButton*)view_ref_ cell] controlSize] == NSSmallControlSize)
    return Spacing(-3, -5, -6, -5);
  else
    return Spacing(0, -6, -4, -6);
}

void Cocoa::Checkbox::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];
  [(NSButton*)view_ref_ setButtonType:NSSwitchButton];
  if (properties.Exists(kPropText))
    [(NSButton*)view_ref_ setTitle:
        NSStringWithString(properties[kPropText].Coerce<String>())];
  MakeTarget();
  ConfigureView();
}

void Cocoa::Checkbox::Finalize() {
  // Setting this in InitializeProperties doesn't work.
  [(NSButton*)view_ref_ setState:NSOffState];
}

bool Cocoa::Checkbox::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropValue) == 0) {
    [(NSButton*)view_ref_ setState:value.Coerce<bool>() ?
        NSOnState : NSOffState];
  }
  return Control::SetProperty(name, value);
}

Value Cocoa::Checkbox::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropPadding) == 0) {
    // AHIG recommends 8 pixels for normal/small, but Interface Builder uses 6.
    if ([[(NSButton*)view_ref_ cell] controlSize] == NSMiniControlSize)
      return Spacing(5, 5, 5, 5);
    else  // regular and small
      return Spacing(6, 6, 6, 6);
  }
  if (strcmp(name, kPropValue) == 0) {
    return static_cast<int32_t>([(NSButton*)view_ref_ state]);
  }
  return Control::GetProperty(name);
}

void Cocoa::Label::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];
  [(NSTextField*)view_ref_ setBezeled:NO];
  [(NSTextField*)view_ref_ setDrawsBackground:NO];
  [(NSTextField*)view_ref_ setEditable:NO];
  if (properties.Exists(kPropText))
    [(NSTextField*)view_ref_ setStringValue:
        NSStringWithString(properties[kPropText].Coerce<String>())];
  if (properties.Exists(kPropTextAlign)) {
    NSTextAlignment align = NSNaturalTextAlignment;
    const String align_value = properties[kPropTextAlign].Coerce<String>();

    if (align_value == kTextAlignLeft)
      align = NSLeftTextAlignment;
    else if (align_value == kTextAlignCenter)
      align = NSCenterTextAlignment;
    else if (align_value == kTextAlignRight)
      align = NSRightTextAlignment;
    [(NSTextField*)view_ref_ setAlignment:align];
  }
  ConfigureView();
}

Value Cocoa::Label::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropPadding) == 0) {
    // AHIG recommends 8/6/5, IB uses 8
    switch ([[(NSControl*)view_ref_ cell] controlSize]) {
      case NSRegularControlSize:
        return Spacing(8, 8, 8, 8);
      case NSSmallControlSize:
        return Spacing(6, 6, 6, 6);
      case NSMiniControlSize:
        return Spacing(5, 5, 5, 5);
      default:
        return Value();
    }
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    Layout *layout = entity_->GetLayout();

    if (layout == NULL)
      return Size();
    if (layout->GetHSizeOption() == kSizeFill) {
      NSString *text = [(NSTextField*)view_ref_ stringValue];
      CGFloat width = 0.0f, height = 0.0f, baseline = 0.0f;
      HIThemeTextInfo info = { 1,
          kThemeStateActive,
          kThemeSystemFont,
          kHIThemeTextHorizontalFlushDefault,
          kHIThemeTextVerticalFlushDefault,
          0, 0, 0, false, 0, NULL };

      HIThemeGetTextDimensions(
          (CFTypeRef)text, [view_ref_ bounds].size.width,
          &info, &width, &height, &baseline);
      if (height != [view_ref_ bounds].size.height)
        entity_->GetLayout()->GetLayoutParent()->InvalidateLayout();
      return Size(width, height);
    } else {
      const NSSize cell_size = [[(NSControl*)view_ref_ cell] cellSize];

      return Size(cell_size.width+1, cell_size.height);
    }
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 13;  // depending on UI size
  }
  return Control::GetProperty(name);
}

bool Cocoa::Label::SetProperty(const PropertyName name, const Value &value) {
  if (strcmp(name, kPropSize) == 0) {
    if (entity_->GetLayout()->GetHSizeOption() == kSizeFill) {
      const NSSize old_size = [view_ref_ bounds].size;

      Control::SetProperty(name, value);
      if (!NSEqualSizes([view_ref_ bounds].size, old_size)) {
        LayoutContainer *lp =
            dynamic_cast<LayoutContainer*>(entity_->GetParent());

        if (lp != NULL)
          lp->InvalidateLayout();
      }
      return true;
    } else {
      return Control::SetProperty(name, value);
    }
  }
  return Control::SetProperty(name, value);
}

Spacing Cocoa::Label::GetInset() const {
  return Spacing(0, -1, 0, 0);
}

void Cocoa::Link::InitializeProperties(const PropertyMap &properties) {
  Label::InitializeProperties(properties);
  [(NSTextField*)view_ref_ setAllowsEditingTextAttributes:YES];
  [(NSTextField*)view_ref_ setSelectable:YES];
  if (properties.Exists(kPropURL)) {
    SetURL(properties[kPropURL].Coerce<String>());
  }
}

bool Cocoa::Link::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropURL) == 0) {
    SetURL(value.Coerce<String>());
    return true;
  }
  return Label::SetProperty(name, value);
}

void Cocoa::Link::SetURL(const String &url) {
  // Explicity set the underline and font attributes, or else they will change
  // after a click
  NSDictionary *attr = [NSDictionary dictionaryWithObjectsAndKeys:
      NSStringWithString(url), NSLinkAttributeName,
      [NSColor blueColor], NSForegroundColorAttributeName,
      [NSNumber numberWithInt:NSUnderlineStyleSingle],
          NSUnderlineStyleAttributeName,
      [NSFont systemFontOfSize:[NSFont systemFontSize]], NSFontAttributeName,
      nil];
  NSAttributedString *string = [[NSAttributedString alloc]
      initWithString:[(NSTextField*)view_ref_ stringValue]
          attributes:attr];

  [(NSTextField*)view_ref_ setAttributedStringValue:string];
  [string release];
}

void Cocoa::EditField::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[GetTextFieldClass() alloc]
      initWithFrame:NSMakeRect(0, 0, 50, 20)];
  if (properties.Exists(kPropText))
    [(NSTextField*)view_ref_ setStringValue:
        NSStringWithString(properties[kPropText].Coerce<String>())];
  ConfigureView();
}

Class Cocoa::EditField::GetTextFieldClass() {
  return [NSTextField class];
}

Value Cocoa::EditField::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    // AHIG and IB agree
    return Spacing(10, 8, 10, 8);
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 16;
  }
  return Control::GetProperty(name);
}

bool Cocoa::EditField::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropText) == 0) {
    [(NSTextField*)view_ref_ setStringValue:
        NSStringWithString(value.Coerce<String>())];
    return true;
  }
  return Control::SetProperty(name, value);
}

Class Cocoa::PasswordField::GetTextFieldClass() {
  return [NSSecureTextField class];
}

void Cocoa::PathBox::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[PathBoxControl alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];
  ConfigureView();
}

Value Cocoa::PathBox::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    // Use same as edit field
    return Spacing(10, 8, 10, 8);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Size(20, 20);
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 15;
  }
  if (strcmp(name, kPropText) == 0) {
    return String([[(PathBoxControl*)view_ref_ path] UTF8String]);
  }
  return View::GetProperty(name);
}

bool Cocoa::PathBox::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropText) == 0) {
    [(PathBoxControl*)view_ref_ setPath:
        NSStringWithString(value.Coerce<String>())];
    return true;
  }
  return View::SetProperty(name, value);
}

void Cocoa::Separator::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
  ConfigureView();
}

// TODO(catmull): Find a cross-platform place for this
void Cocoa::Separator::Finalize() {
  Layout *layout = entity_->GetLayout();

  if (layout == NULL)
    return;
  if (layout->GetDirection() == Layout::kLayoutRow)
    layout->SetVSizeOption(kSizeFill);
  else  // kLayoutColumn
    layout->SetHSizeOption(kSizeFill);
}

Value Cocoa::Separator::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Size(2, 2);
  }
  if (strcmp(name, kPropPadding) == 0) {
    Layout *layout = entity_->GetLayout();

    if (layout == NULL)
      return Value();
    if (layout->GetDirection() == Layout::kLayoutRow)
      return Spacing(2, 10, 2, 10);
    else  // kLayoutColumn
      return Spacing(10, 2, 10, 2);
  }
  return Control::GetProperty(name);
}

void Cocoa::Image::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, 20, 20)];
  [(NSImageView*)view_ref_ setImageScaling:NSScaleProportionally];
  if (properties.Exists(kPropFile)) {
    NSString *file_name =
        NSStringWithString(properties[kPropFile].Coerce<String>());
    NSImage *image = [NSImage imageNamed:file_name];

    if (image != nil)
      [(NSImageView*)view_ref_ setImage:image];
  }
  ConfigureView();
}

Value Cocoa::Image::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMinimumSize) == 0) {
    if ([(NSImageView*)view_ref_ image] == nil)
      return Size(20, 20);

    const NSSize image_size = [[(NSImageView*)view_ref_ image] size];

    return Size(image_size.width, image_size.height);
  }
  return Control::GetProperty(name);
}

void Cocoa::Popup::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSPopUpButton alloc]
      initWithFrame:NSMakeRect(0, 0, 50, 20)
          pullsDown:NO];
  ConfigureView();
}

Value Cocoa::Popup::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    // Using AHIG spacing.
    switch ([[(NSControl*)view_ref_ cell] controlSize]) {
      case NSRegularControlSize:
        return Spacing(10, 8, 10, 8);
      case NSSmallControlSize:
        return Spacing(8, 6, 8, 6);
      case NSMiniControlSize:
        return Spacing(6, 5, 6, 5);
      default:
        return Value();
    }
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 15;  // depending on UI size
  }
  return Control::GetProperty(name);
}

Spacing Cocoa::Popup::GetInset() const {
  return Spacing(-2, -3, -3, -3);
}

void Cocoa::Popup::AddChild(Native *child) {
  ScopedAutoreleasePool pool;

  if ([(id)child->GetNativeRef() isKindOfClass:[NSMenuItem class]])
    [[view_ref_ menu] addItem:(NSMenuItem*)child->GetNativeRef()];
}

void Cocoa::PopupItem::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  NSString *title = @"";

  if (properties.Exists(kPropText))
    title = NSStringWithString(properties[kPropText].Coerce<String>());
  item_ = [[NSMenuItem alloc]
      initWithTitle:title action:NULL keyEquivalent:@""];
}

bool Cocoa::PopupItem::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;

    [item_ setEnabled:value.Coerce<bool>()];
    return true;
  }
  return NativeCocoa::SetProperty(name, value);
}

Value Cocoa::PopupItem::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;
    return (bool)[item_ isEnabled];
  }
  return NativeCocoa::GetProperty(name);
}

} // Diadem
