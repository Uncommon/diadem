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
}

- (void)setURL:(NSURL *)url;
- (NSURL*)URL;

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

- (void)setURL:(NSURL *)url {
  [pathControl_ setURL:url];
}

- (NSURL*)URL {
  return [pathControl_ URL];
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
  factory->RegisterNative<Label>("label");
  factory->RegisterNative<Link>("link");
  factory->RegisterNative<Window>("window");
  factory->RegisterNative<Button>("button");
  factory->RegisterNative<Checkbox>("check");
  factory->RegisterNative<EditField>("edit");
  factory->RegisterNative<PasswordField>("password");
  factory->RegisterNative<PathBox>("path");
  factory->RegisterNative<Separator>("separator");
  factory->RegisterNative<Image>("image");
  factory->RegisterNative<Popup>("popup");
  factory->RegisterNative<PopupItem>("item");
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

void Cocoa::Window::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  window_ref_ = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 50, 50)
                styleMask:NSTitledWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window_ref_ setContentMinSize:NSMakeSize(10, 10)];
  if ((window_ref_ != nil) && properties.Exists(Entity::kPropText)) {
    const String title_string = properties[Entity::kPropText].Coerce<String>();

    [window_ref_ setTitle:NSStringWithString(title_string)];
  }
}

Cocoa::Window::~Window() {
  ScopedAutoreleasePool pool;

  Close();
  [window_ref_ release];
}

Bool Cocoa::Window::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (window_ref_ == nil)
    return false;
  if (strcmp(name, Entity::kPropText) == 0) {
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
  if (strcmp(name, Entity::kPropText) == 0) {
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
    return Value((Bool)[window_ref_ isVisible]);
  }
  return Value();
}

void Cocoa::Window::AddChild(Native *child) {
  ScopedAutoreleasePool pool;

  if (![(id)child->GetNativeRef() isKindOfClass:[NSView class]])
    return;

  [[window_ref_ contentView] addSubview:(NSView*)child->GetNativeRef()];
}

Bool Cocoa::Window::ShowModeless() {
  ScopedAutoreleasePool pool;

  [window_ref_ center];
  [window_ref_ makeKeyAndOrderFront:nil];
  return true;
}

Bool Cocoa::Window::Close() {
  ScopedAutoreleasePool pool;

  if ([NSApp modalWindow] == window_ref_)
    [NSApp abortModal];
  [window_ref_ orderOut:nil];
  return true;
}

Bool Cocoa::Window::ShowModal(void*) {
  ScopedAutoreleasePool pool;

  [NSApp runModalForWindow:window_ref_];
  return true;
}

Bool Cocoa::Window::EndModal() {
  ScopedAutoreleasePool pool;

  if ([NSApp modalWindow] != window_ref_)
    return false;
  [NSApp stopModal];
  return true;
}

Bool Cocoa::Window::SetFocus(Entity *new_focus) {
  if ((new_focus->GetNative() == NULL) ||
      (new_focus->GetNative()->GetNativeRef() == NULL))
    return false;

  id focus = (id)new_focus->GetNative()->GetNativeRef();

  if (![focus isKindOfClass:[NSResponder class]])
    return false;
  [window_ref_ makeFirstResponder:focus];
  return true;
}

void Cocoa::View::ConfigureView() {
  // The view needs to be anchored tho the top left of its parent
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

Bool Cocoa::View::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (view_ref_ == NULL)
    return false;
  if (strcmp(name, kPropLocation) == 0) {
    const Spacing inset = GetInset();
    const Location offset = GetViewOffset();
    const Location loc = value.Coerce<Location>() +
        offset + Location(inset.left, inset.top);
    NSPoint origin = InvertPoint(NSMakePoint(loc.x, loc.y));

    [[view_ref_ superview] setNeedsDisplayInRect:[view_ref_ frame]];
    origin.y -= [view_ref_ frame].size.height;
    [view_ref_ setFrameOrigin:origin];
    [view_ref_ setNeedsDisplay:YES];
    DASSERT(
        value.Coerce<Location>() ==
        GetProperty(kPropLocation).Coerce<Location>());
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
    [view_ref_ setHidden:!value.Coerce<Bool>()];
    return true;
  }
  return false;
}

Value Cocoa::View::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (view_ref_ == nil)
    return Value();
  if (strcmp(name, kPropLocation) == 0) {
    const NSPoint location = InvertPoint([view_ref_ frame].origin);
    const Spacing inset = GetInset();

    return Location(location.x, location.y - [view_ref_ frame].size.height) -
        GetViewOffset() - Location(inset.left, inset.top);
  }
  if (strcmp(name, kPropSize) == 0) {
    return GetSize() + GetInset();
  }
  if (strcmp(name, kPropVisible) == 0) {
    return (Bool)![view_ref_ isHidden];
  }
  return Value();
}

Size Cocoa::View::GetSize() const {
  ScopedAutoreleasePool pool;
  const NSRect frame = [view_ref_ frame];

  return Size(frame.size.width, frame.size.height);
}

Bool Cocoa::Control::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, Entity::kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;

    [(NSControl*)view_ref_ setEnabled:value.Coerce<Bool>()];
    return true;
  }
  return View::SetProperty(name, value);
}

// -cellSize is documented to return this in some cases
static NSSize kMaxCellSize = { 10000, 10000 };

Value Cocoa::Control::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropMinimumSize) == 0) {
    const NSSize cell_size = [[(NSControl*)view_ref_ cell] cellSize];

    if (NSEqualSizes(cell_size, NSZeroSize) ||
        NSEqualSizes(cell_size, kMaxCellSize))
      return GetProperty(kPropSize);

    return Size(cell_size.width, cell_size.height) + GetInset();
  }
  if (strcmp(name, Entity::kPropText) == 0) {
    NSString *text = [(NSControl*)view_ref_ stringValue];

    return String([text UTF8String]);
  }
  if (strcmp(name, Entity::kPropEnabled) == 0) {
    return (Bool)[(NSControl*)view_ref_ isEnabled];
  }
  return View::GetProperty(name);
}

void Cocoa::Button::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  NSButton *button = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];

  view_ref_ = button;
  [button setBezelStyle:NSRoundedBezelStyle];
  [button setButtonType:NSMomentaryPushInButton];
  [button setImagePosition:NSNoImage];
  [button setBordered:YES];
  if (properties.Exists(Entity::kPropText))
    [button setTitle:
        NSStringWithString(properties[Entity::kPropText].Coerce<String>())];
  if (properties.Exists(kPropUISize)) {
    const String ui_size = properties[kPropUISize].Coerce<String>();

    if (ui_size == "small") {
      [[button cell] setControlSize:NSSmallControlSize];
      [[button cell] setFont:
          [NSFont systemFontOfSize:
              [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
    }
  }
  target_ = [[ButtonTarget alloc] initWithButton:this];
  ConfigureView();
}

Cocoa::Button::~Button() {
  ScopedAutoreleasePool pool;

  [target_ release];
}

Bool Cocoa::Button::SetProperty(PropertyName name, const Value &value) {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropButtonType) == 0) {
    const String type = value.Coerce<String>();

    if (type == "default")
      [(NSButton*)view_ref_ setKeyEquivalent:@"\r"];
    else if (type == "cancel") {
      [(NSButton*)view_ref_ setKeyEquivalent:@"."];
      [(NSButton*)view_ref_ setKeyEquivalentModifierMask:NSCommandKeyMask];
    }
    return true;
  }

  return Control::SetProperty(name, value);
}

#define kButtonWidthAdjustment 12

Value Cocoa::Button::GetProperty(PropertyName name) const {
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
    if ([[(NSButton*)view_ref_ cell] controlSize] == NSSmallControlSize)
      return Spacing(10, 10, 10, 10);
    else
      return Spacing(12, 12, 12, 12);
  }
  if (strcmp(name, Entity::kPropText) == 0) {
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

Spacing Cocoa::Button::GetInset() const {
  // extra space on botom for the shadow, apparently
  // the need for the left/right inset is mysterious
  if ([[(NSButton*)view_ref_ cell] controlSize] == NSSmallControlSize)
    return Spacing(-3, -5, -6, -5);
  else
    return Spacing(0, -6, -4, -6);
}

void Cocoa::Checkbox::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];
  [(NSButton*)view_ref_ setButtonType:NSSwitchButton];
  if (properties.Exists(Entity::kPropText))
    [(NSButton*)view_ref_ setTitle:
        NSStringWithString(properties[Entity::kPropText].Coerce<String>())];
  ConfigureView();
}

void Cocoa::Label::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;

  view_ref_ = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 50, 20)];
  [(NSTextField*)view_ref_ setBezeled:NO];
  [(NSTextField*)view_ref_ setDrawsBackground:NO];
  [(NSTextField*)view_ref_ setEditable:NO];
  if (properties.Exists(Entity::kPropText))
    [(NSTextField*)view_ref_ setStringValue:
        NSStringWithString(properties[Entity::kPropText].Coerce<String>())];
  if (properties.Exists(kPropTextAlign)) {
    NSTextAlignment align = NSNaturalTextAlignment;
    const String align_value = properties[kPropTextAlign].Coerce<String>();

    if (align_value == "left")
      align = NSLeftTextAlignment;
    else if (align_value == "center")
      align = NSCenterTextAlignment;
    else if (align_value == "right")
      align = NSRightTextAlignment;
    [(NSTextField*)view_ref_ setAlignment:align];
  }
  ConfigureView();
}

Value Cocoa::Label::GetProperty(PropertyName name) const {
  ScopedAutoreleasePool pool;

  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(8, 8, 8, 8);
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

Bool Cocoa::Label::SetProperty(const PropertyName name, const Value &value) {
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

Bool Cocoa::Link::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropURL) == 0) {
    SetURL(value.Coerce<String>());
    return true;
  }
  return Label::SetProperty(name, value);
}

void Cocoa::Link::SetURL(const String &url) {
  NSDictionary *attr = [NSDictionary dictionaryWithObjectsAndKeys:
      NSStringWithString(url), NSLinkAttributeName,
      [NSColor blueColor], NSForegroundColorAttributeName,
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
  if (properties.Exists(Entity::kPropText))
    [(NSTextField*)view_ref_ setStringValue:
        NSStringWithString(properties[Entity::kPropText].Coerce<String>())];
  ConfigureView();
}

Class Cocoa::EditField::GetTextFieldClass() {
  return [NSTextField class];
}

Value Cocoa::EditField::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(10, 10, 10, 10);
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 16;
  }
  return Control::GetProperty(name);
}

Bool Cocoa::EditField::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, Entity::kPropText) == 0) {
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
  if (properties.Exists(Entity::kPropText))
    [(PathBoxControl*)view_ref_ setURL:[NSURL fileURLWithPath:
        NSStringWithString(properties[Entity::kPropText].Coerce<String>())]];
  ConfigureView();
}

Value Cocoa::PathBox::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(10, 10, 10, 10);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Size(20, 20);
  }
  if (strcmp(name, kPropBaseline) == 0) {
    return 15;
  }
  if (strcmp(name, Entity::kPropText) == 0) {
    return String([[[(NSPathControl*)view_ref_ URL] path] UTF8String]);
  }
  return View::GetProperty(name);
}

Bool Cocoa::PathBox::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, Entity::kPropText) == 0) {
    [(NSPathControl*)view_ref_ setURL:
        [NSURL fileURLWithPath:NSStringWithString(value.Coerce<String>())]];
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

  LayoutContainer *layoutParent = layout->GetLayoutParent();

  if (layoutParent == NULL)
    return;
  if (layoutParent->GetDirection() == LayoutContainer::kLayoutRow)
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

    LayoutContainer *layoutParent = layout->GetLayoutParent();

    if (layoutParent == NULL)
      return Value();
    if (layoutParent->GetDirection() == LayoutContainer::kLayoutRow)
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
    return Spacing(10, 8, 10, 8);
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
  if ([(id)child->GetNativeRef() isKindOfClass:[NSMenuItem class]])
    [[view_ref_ menu] addItem:(NSMenuItem*)child->GetNativeRef()];
}

void Cocoa::PopupItem::InitializeProperties(const PropertyMap &properties) {
  ScopedAutoreleasePool pool;
  NSString *title = @"";

  if (properties.Exists(Entity::kPropText))
    title = NSStringWithString(properties[Entity::kPropText].Coerce<String>());
  item_ = [[NSMenuItem alloc]
      initWithTitle:title action:NULL keyEquivalent:@""];
}

Bool Cocoa::PopupItem::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, Entity::kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;

    [item_ setEnabled:value.Coerce<Bool>()];
    return true;
  }
  return NativeCocoa::SetProperty(name, value);
}

Value Cocoa::PopupItem::GetProperty(PropertyName name) const {
  if (strcmp(name, Entity::kPropEnabled) == 0) {
    ScopedAutoreleasePool pool;
    return (Bool)[item_ isEnabled];
  }
  return NativeCocoa::GetProperty(name);
}

} // Diadem
