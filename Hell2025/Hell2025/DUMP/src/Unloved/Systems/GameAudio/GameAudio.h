#pragma once

namespace Unloved::GameAudio {
    void BeginFrame();
    void Update();

    void PlayGlassHitAudio();
    void PlayFootstepIndoorAudio();
    void PlayFootstepOutdoorAudio();
    void TryPlayFleshImpactAudio();
}
