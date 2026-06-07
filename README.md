# 20-20 Break

A very small native Windows app written in C. It runs in the background, waits 20 minutes, shows a notification, opens its reminder window, counts down a 20 second break, then hides again.

Only one instance can run at a time. If you open it again, the already-running window is brought to the front.

## Build

Run:

```bat
build.bat
```

This uses MinGW `gcc` and `windres`.

## Use

Run `break-reminder.exe`.

- The app starts hidden in the system tray.
- Double-click the tray icon to open the window.
- Right-click the tray icon for Open and Exit.
- Closing the window hides it; use Exit from the tray menu to quit.
