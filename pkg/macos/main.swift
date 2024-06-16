//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Launcher program that runs the main wadgadget executable in the user's
// preferred terminal program. It also saves any files that the user
// requested to open to a temporary text file; the main executable will
// then read this file and use it as a set of initial WAD file(s) to open.
// This allows the program to open WAD files through the Finder. This is
// a very hacky way of passing the filenames through to the main program,
// but is the best/only way I've found to do it.

import Cocoa
import os.log

class AppDelegate: NSObject, NSApplicationDelegate {

	var window: NSWindow?

	func applicationWillFinishLaunching(_ aNotification: Notification) {
		Logger().log("applicationWillFinishLaunching")
		do {
			try FileManager.default.removeItem(
				atPath: "/tmp/wadgadget-paths.txt")
		} catch {
			Logger().log("failed to delete paths file")
		}
	}

	func application(_ sender: NSApplication, openFiles filenames: [String]) {
		Logger().log("openFiles")
		var lines = ""
		for filename in filenames {
			lines = lines + filename + "\n"
		}
		do {
			try lines.write(toFile: "/tmp/wadgadget-paths.txt",
			                atomically: true,
			                encoding: String.Encoding.utf8)
		} catch {
			Logger().log("failed to write paths file")
		}
	}

	func applicationDidFinishLaunching(_ aNotification: Notification) {
		Logger().log("applicationDidFinishLaunching")
		let dir = (appExecutable as NSString).deletingLastPathComponent
		let path = dir + "/wadgadget"
		let url = URL(fileURLWithPath: path)
		dummyRunMainProgram(url)

		// Open the main executable. Since it is of the unix-executable
		// type, macOS will launch the user's preferred terminal
		// program to open it. Unfortunately we can't use
		// NSWorkspace.OpenConfiguration.arguments to pass through the
		// WAD file paths; if we do, these are passed as command line
		// arguments to Terminal.app, not the program it launches.
		NSWorkspace.shared.open(url)

		// We're done. There's no reason to keep running.
		NSApplication.shared.terminate(self)
	}

	// If this program is running, the user has granted permission for this
	// app to run. We need to pass that permission to the main executable
	// before we launch it in the terminal, otherwise the same permission
	// error will occur again.
	private func dummyRunMainProgram(_ url: URL) {
		do {
			try Process.run(url, arguments: ["--version"])
		} catch {
			Logger().log("failed dummy run of main program")
		}
	}
}

let appDelegate = AppDelegate()
let application = NSApplication.shared
application.delegate = appDelegate

let appExecutable = CommandLine.arguments[0]
let _ = NSApplicationMain(CommandLine.argc, CommandLine.unsafeArgv)

