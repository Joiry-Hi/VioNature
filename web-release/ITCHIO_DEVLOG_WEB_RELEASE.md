# VioNature Is Now Playable In The Browser

VioNature now has an HTML5/WebAssembly build.

That means you can launch the game directly from the itch.io page without downloading a desktop archive first. It is still the same strange, fast, highly configurable FPS laboratory: nine weapons, arena movement, drones, magic circles, boss fights, spherical maps, hollow worlds, wormholes, time stop, black holes, and a frankly unreasonable number of tunable parameters.

The difference is that it now fits inside a browser tab.

## What Changed

This update is mostly a release-platform update, not a balance patch. The goal was to make the full desktop game survive the trip to the web.

The new browser build includes:

- A proper itch.io HTML5 package
- Automatic 16:9 canvas scaling
- Browser fullscreen support
- Pointer-lock mouse look for FPS controls
- Preloaded assets and config files through WebAssembly packaging
- The same gameplay systems as the native desktop build

The web version should automatically scale to the available page or iframe size. If the embed area is not exactly 16:9, the game will letterbox instead of cutting off the HUD or weapon model.

## How To Play The Web Version

Click **Run Game**, wait for the game to load, then click the game canvas once.

That click is important: browsers require a user gesture before a game can enter fullscreen or lock the mouse for FPS controls. After clicking, the game will try to enter fullscreen and capture mouse movement.

If mouse look stops working after pressing Esc, switching tabs, or leaving fullscreen, just click the game canvas again.

## A Small Browser Adventure

Getting VioNature onto the web was not as simple as "compile it and upload it."

The native game expects a normal C++ startup path, a persistent game object, desktop-style mouse delta, a fixed render target, and direct file access. The browser build needed a few custom pieces:

- The page now explicitly starts the game after the WebAssembly runtime is ready.
- The game assets are packed into a `.data` file so the browser can load models, sounds, fonts, and config files.
- Mouse look uses browser `movementX/Y` while pointer lock is active.
- The canvas is resized by the HTML shell while the game keeps its internal 16:9 render logic.

The result is a build that behaves much closer to the desktop version than the first rough web attempt.

## Mobile Note

This is not a proper mobile port yet.

However, during testing, touch-dragging on a phone could already rotate the camera in some browsers. There is no virtual joystick, no touch fire button, and no mobile HUD yet, so it is more of a curious experiment than a supported way to play.

Still, it is a fun sign that a lightweight mobile-control prototype might be possible someday.

## Known Notes

- For best controls, use keyboard and mouse.
- Click the canvas once after loading to activate fullscreen and mouse look.
- If the game appears letterboxed, that is intentional: it preserves the full 16:9 view.
- The browser build may behave differently depending on browser fullscreen and pointer-lock permissions.
- The native desktop build remains available for the most stable experience.

## Why This Matters

VioNature has gradually become less of a conventional arena shooter and more of a playable FPS experiment space. It is a place to test movement chains, impossible weapons, weird map physics, mirrored world layers, and live-tuned parameters.

Making it playable in the browser lowers the friction a lot. You can now open the page, jump into the arena, and immediately start poking at the systems.

That feels right for this project.

Thanks for playing, testing, and giving this strange little FPS laboratory room to mutate.
