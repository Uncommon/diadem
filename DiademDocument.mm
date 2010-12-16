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

#import "DiademDocument.h"
#include "Diadem/Entity.h"
#include "Diadem/Factory.h"
#include "Diadem/LibXMLParser.h"
#include "Diadem/NativeCocoa.h"

@interface DiademController : NSWindowController {
 @private
  Diadem::Entity *entity_;
}

- (id)initWithEntity:(Diadem::Entity*)entity;
- (Diadem::Entity*)entity;
- (void)setEntity:(Diadem::Entity*)entity;

@end

@interface OverlayView : NSView {
 @private
  Diadem::Entity *entity_;
}

- (id)initWithFrame:(NSRect)frame entity:(Diadem::Entity*)entity;

@end

@implementation DiademDocument

- (void)makeOverlayWindow {
  DiademController *mainController = (DiademController*)
      [[self windowControllers] objectAtIndex:0];
  NSWindow *mainWindow = [mainController window];
  const NSRect contentRect =
      [mainWindow contentRectForFrameRect:[mainWindow frame]];
  const NSRect viewRect = { { 0, 0 }, contentRect.size };

  overlayWindow_ = [[NSWindow alloc]
      initWithContentRect:contentRect
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [overlayWindow_ setBackgroundColor:[NSColor clearColor]];
  [overlayWindow_ setIgnoresMouseEvents:YES];
  [overlayWindow_ setHasShadow:NO];
  [overlayWindow_ setOpaque:NO];
  [overlayWindow_ setAlphaValue:0.0];
  [[overlayWindow_ contentView] addSubview:
      [[[OverlayView alloc] initWithFrame:viewRect
                                   entity:[mainController entity]]
          autorelease]];
  [overlayWindow_ orderFront:self];
  [mainWindow addChildWindow:overlayWindow_ ordered:NSWindowAbove];
}

- (BOOL)readFromData:(NSData *)data
              ofType:(NSString *)typeName
               error:(NSError **)outError {
  NSMutableData *terminatedData = [data mutableCopy];

  [terminatedData appendBytes:"" length:1];

  Diadem::Factory factory;

  Diadem::Cocoa::SetUpFactory(&factory);

  Diadem::LibXMLParser parser(factory);
  Diadem::Entity *entity =
      parser.LoadEntityFromData((const char*)[terminatedData bytes]);

  if (entity == NULL)
    return NO;
  if ((entity->GetNative() == NULL) ||
      (entity->GetNative()->GetNativeRef() == NULL)) {
    delete entity;
    return NO;
  }

  if ([[self windowControllers] count] == 0) {
    [self addWindowController:
        [[[DiademController alloc] initWithEntity:entity] autorelease]];
  } else {
    [overlayWindow_ release];
    [(DiademController*)[[self windowControllers] objectAtIndex:0]
        setEntity:entity];
  }
  [self makeOverlayWindow];

  return YES;
}

- (IBAction)reload:(id)sender {
  NSError *error = nil;

  [self revertToContentsOfURL:[self fileURL]
                       ofType:[self fileType]
                        error:&error];
}

- (IBAction)toggleFrames:(id)sender {
  if ([overlayWindow_ alphaValue] > 0.0)
    [overlayWindow_ setAlphaValue:0.0];
  else
    [overlayWindow_ setAlphaValue:1.0];
}

- (void)dealloc {
  [overlayWindow_ release];
  [super dealloc];
}

@end

@implementation DiademController

- (id)initWithEntity:(Diadem::Entity*)entity {
  if ([super init] == nil)
    return nil;
  entity_ = entity;
  [self setWindow:(NSWindow*)entity->GetNative()->GetNativeRef()];
  [[self window] center];
  return self;
}

- (void)dealloc {
  [self setWindow:nil];
  delete entity_;

  [super dealloc];
}

- (Diadem::Entity*)entity {
  return entity_;
}

- (void)setEntity:(Diadem::Entity*)entity {
  [self setWindow:(NSWindow*)entity->GetNative()->GetNativeRef()];
  delete entity_;
  entity_ = entity;

  [[self window] center];
  [[self window] makeKeyAndOrderFront:self];
}

@end

@implementation OverlayView

- (id)initWithFrame:(NSRect)frame entity:(Diadem::Entity*)entity {
  if ([super initWithFrame:frame] == nil)
    return nil;

  entity_ = entity;
  [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  return self;
}

- (BOOL)isFlipped { return YES; }

- (void)drawEntityFrame:(Diadem::Entity*)entity
                 offset:(Diadem::Location)offset {
  if (entity->GetLayout() == NULL)
    return;
  if ((entity != entity_) &&
      !entity->GetProperty(Diadem::kPropVisible).Coerce<Diadem::Bool>())
    return;

  const Diadem::Location loc = (entity->GetParent() == NULL) ?
      Diadem::Location() :
      (entity->GetProperty(Diadem::kPropLocation).Coerce<Diadem::Location>() +
       offset);
  const Diadem::Size size =
      entity->GetProperty(Diadem::kPropSize).Coerce<Diadem::Size>();

  const Diadem::Spacing pad =
      entity->GetProperty(Diadem::kPropPadding).Coerce<Diadem::Spacing>();
  NSRect frame = {
      { loc.x + 0.5, loc.y + 0.5 },
      { size.width - 1, size.height - 1 } };
  NSGraphicsContext *context = [NSGraphicsContext currentContext];

  // Frame
  [context saveGraphicsState];
  [[NSColor colorWithDeviceRed:1 green:0 blue:0 alpha:1] setStroke];
  [NSBezierPath strokeRect:frame];

  // View frame
  if ((entity->GetNative() != NULL) &&
      [(id)entity->GetNative()->GetNativeRef() isKindOfClass:[NSView class]]) {
    NSView *view = (NSView*)entity->GetNative()->GetNativeRef();
    NSView *content = [[view window] contentView];

    if ([view isKindOfClass:[NSView class]] && ([view superview] != nil)) {
      NSRect frame = NSOffsetRect(
          [[view superview] convertRect:[view frame] toView:content],
          0.5, 0.5);
      const NSRect contentFrame = [content frame];

      frame.origin.y =
          contentFrame.size.height - frame.origin.y - frame.size.height;
      [[NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:0.25] setStroke];
      [NSBezierPath strokeRect:frame];
    }
  }

  // Padding
  if (pad != Diadem::Spacing()) {
    NSRect padFrame = {
        { frame.origin.x - pad.left - 0.5,
          frame.origin.y - pad.top - 0.5 },
        { frame.size.width  + (pad.left + pad.right) + 1,
          frame.size.height + (pad.top + pad.bottom) + 1 } };
    NSBezierPath *padPath = [[[NSBezierPath alloc] init] autorelease];

    [[NSColor colorWithDeviceRed:0 green:0 blue:1 alpha:0.2] setFill];
    [padPath appendBezierPathWithRect:padFrame];
    [padPath appendBezierPathWithRect:frame];
    [padPath setWindingRule:NSEvenOddWindingRule];
    [padPath fill];
  }

  // Baseline
  if (entity->ChildrenCount() == 0) {
    const int32_t baseline =
        entity->GetProperty(Diadem::kPropBaseline).Coerce<int32_t>();

    if (baseline > 0) {
      NSPoint points[2] = {
          { frame.origin.x - 2,
            frame.origin.y + baseline },
          { frame.origin.x + frame.size.width + 2,
            frame.origin.y + baseline } };
      NSBezierPath *baselinePath = [[[NSBezierPath alloc] init] autorelease];

      [[NSColor colorWithDeviceRed:0 green:0.6 blue:0 alpha:0.6] setStroke];
      [baselinePath appendBezierPathWithPoints:points count:2];
      [baselinePath stroke];
    }
  }

  // Spacer

  // Container
  const Diadem::Spacing margins =
      entity->GetProperty(Diadem::kPropMargins).Coerce<Diadem::Spacing>();

  if (margins != Diadem::Spacing()) {
    const NSRect marginFrame = {
        { frame.origin.x + margins.left - 0.5,
          frame.origin.y + margins.top - 0.5 },
        { frame.size.width  - (margins.left + margins.right) + 1,
          frame.size.height - (margins.top + margins.bottom) + 1 } };
    NSBezierPath *marginsPath = [[[NSBezierPath alloc] init] autorelease];

    [[NSColor colorWithDeviceRed:0 green:0.5 blue:0 alpha:0.2] setFill];
    [marginsPath appendBezierPathWithRect:frame];
    [marginsPath appendBezierPathWithRect:marginFrame];
    [marginsPath setWindingRule:NSEvenOddWindingRule];
    [marginsPath fill];
  }

  for (uint32_t i = 0; i < entity->ChildrenCount(); ++i)
    [self drawEntityFrame:entity->ChildAt(i) offset:loc];
  [context restoreGraphicsState];
}

- (void)drawRect:(NSRect)rect {
  [self drawEntityFrame:entity_ offset:Diadem::Location()];
}

@end
