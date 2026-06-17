// SwiftUI shell entry — open in Xcode alongside the C++ bridge target.
// CompositorLayer invokes vnexr_visionos_run_immersive_session() from VisionOsXrBridge.

import SwiftUI
import CompositorServices

@main
struct VneXrVisionOsApp: App {
    var body: some Scene {
        WindowGroup {
            Text("VneXR visionOS")
        }
        ImmersiveSpace(id: "VneXRImmersive") {
            CompositorLayer { _ in
                vnexr_visionos_run_immersive_session()
            }
        }
    }
}

func vnexr_visionos_run_immersive_session() {
    // Linked from VisionOsXrBridge.mm at build time.
}
