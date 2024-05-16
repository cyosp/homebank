import Foundation
import AppKit

let args = CommandLine.arguments
if args.count != 3 {
    print("[ERROR] Usage: seticon /path/to/icns /path/to/dmg")
    exit(1)
}

var icns = args[1]
var dmg = args[2]

var img = NSImage(byReferencingFile: icns)!
print("Set \(dmg) icon to \(icns)")
if NSWorkspace.shared.setIcon(img, forFile: dmg) {
    print("Done")
} else {
    print("Failed")
    exit(1)
}
