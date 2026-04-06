/*
MIT License

Copyright (c) 2026 Ben Severns

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include "common/EnumTools.hpp"
#include "common/controls/FancyPot.hpp"
#include "common/core/App.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/Memory.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/control/BeatDetector.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/effects/StereoDelay.hpp"
#include "common/dsp/filters/SvfStereo.hpp"
#include "common/dsp/utility/EdgeDetector.hpp"
#include "common/dsp/utility/KastleRungler.hpp"
#include "common/dsp/utility/Portamento.hpp"
#include "common/dsp/utility/Sequencer.hpp"
#include "common/controls/ShiftTrigger.hpp"

namespace kastle2
{

class AppStackBrain : public virtual App
{
public:
    enum class Mode
    {
        STEP,
        FOLLOW,
        CHAOS,
        COUNT
    };

    void Init();
    void DeInit();
    FASTCODE void AudioLoop(q15_t *input, q15_t *output, size_t size);
    void UiLoop();
    void MemoryInitialization();
    void MidiCallback(midi::Message *msg);

    uint8_t GetId()
    {
        return 0x10;
    }

private:
    enum class Pot
    {
        RATE_MOD,
        SHAPE_MOD,
        SLEW,
        RESPONSE,
        RATE,
        RANGE,
        DENSITY,
        PITCH_1_DEPTH,
        PITCH_2_DEPTH,
        LINK,
        EVENT_BIAS,
        MIDI_NOTE_DEPTH,
        MIDI_VELOCITY_DEPTH,
        MIDI_CLOCK_DIV,
        MODE_SELECT,
        CLOCK_DEFAULT,
        LINK_DEFAULT,
        ROUTE_DEFAULT,
        MIDI_ROLE,
        BIAS_ROUTE,
        PERSONALITY,
        COUNT
    };

    enum class ClockDefault
    {
        TRIGGER,
        AUTO,
        MIDI,
        COUNT
    };

    enum class LinkDefault
    {
        INVERT,
        FREE,
        MERGE,
        COUNT
    };

    enum class RouteDefault
    {
        SWAP,
        NORMAL,
        BLEND,
        COUNT
    };

    enum class MidiRole
    {
        OFF,
        BIAS,
        EVENTS,
        HYBRID,
        COUNT
    };

    enum class BiasRoute
    {
        SPLIT,
        CROSS,
        MAIN,
        BOTH,
        COUNT
    };

    enum class Personality
    {
        TAME,
        BALANCED,
        PUSHED,
        WILD,
        COUNT
    };

    static constexpr Hardware::AnalogInput CV_RATE = Hardware::AnalogInput::PARAM_1;
    static constexpr Hardware::AnalogInput CV_SHAPE = Hardware::AnalogInput::PARAM_2;
    static constexpr Hardware::AnalogInput CV_DENSITY = Hardware::AnalogInput::PARAM_3;
    static constexpr Hardware::AnalogInput CV_MODE = Hardware::AnalogInput::MODE;
    static constexpr Hardware::AnalogInput CV_PITCH_MAIN = Hardware::AnalogInput::PITCH_1;
    static constexpr Hardware::AnalogInput CV_PITCH_COMPANION = Hardware::AnalogInput::PITCH_2;
    static constexpr Hardware::AnalogInput FEED_CLOCK = Hardware::AnalogInput::FEED_1;
    static constexpr Hardware::AnalogInput FEED_LINK = Hardware::AnalogInput::FEED_2;
    static constexpr Hardware::AnalogInput FEED_ROUTE = Hardware::AnalogInput::FEED_3;

    static constexpr size_t kMemMode = Memory::ADDR_APP_SPACE + 0x00;
    static constexpr size_t kMemRateMod = Memory::ADDR_APP_SPACE + 0x01;
    static constexpr size_t kMemShapeMod = Memory::ADDR_APP_SPACE + 0x02;
    static constexpr size_t kMemSlew = Memory::ADDR_APP_SPACE + 0x03;
    static constexpr size_t kMemResponse = Memory::ADDR_APP_SPACE + 0x04;
    static constexpr size_t kMemRate = Memory::ADDR_APP_SPACE + 0x05;
    static constexpr size_t kMemRange = Memory::ADDR_APP_SPACE + 0x06;
    static constexpr size_t kMemDensity = Memory::ADDR_APP_SPACE + 0x07;
    static constexpr size_t kMemClockDefault = Memory::ADDR_APP_SPACE + 0x08;
    static constexpr size_t kMemLinkDefault = Memory::ADDR_APP_SPACE + 0x09;
    static constexpr size_t kMemRouteDefault = Memory::ADDR_APP_SPACE + 0x0A;
    static constexpr size_t kMemMidiRole = Memory::ADDR_APP_SPACE + 0x0B;
    static constexpr size_t kMemBiasRoute = Memory::ADDR_APP_SPACE + 0x0C;
    static constexpr size_t kMemPersonality = Memory::ADDR_APP_SPACE + 0x0D;
    static constexpr size_t kMemActiveScene = Memory::ADDR_APP_SPACE + 0x0E;
    static constexpr size_t kMemSceneBase = Memory::ADDR_APP_SPACE + 0x10;
    static constexpr uint8_t kSceneCount = 4;
    static constexpr size_t kSceneStride = static_cast<size_t>(Pot::COUNT);
    static constexpr uint32_t kPotSaveDebounceTicks = 18;
    static constexpr size_t kModeTapTicks = s2alr(0.35f);
    static constexpr size_t kModeShiftTapTicks = s2alr(0.3f);
    static constexpr size_t kSceneFeedbackTicks = s2alr(0.2f);
    static constexpr size_t kSceneMorphTicks = s2alr(0.35f);

    bool inited_ = false;
    Mode current_mode_ = Mode::STEP;

    EnumArray<Pot, std::unique_ptr<FancyPot>> pots_;
    EnumArray<Mode, uint32_t> mode_colors_ = {
        WS2812::GREEN,
        WS2812::BLUE,
        WS2812::ORANGE,
    };

    EdgeDetector trigger_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector reset_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector follow_audio_gate_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector follow_trigger_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector follow_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector step_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    EdgeDetector chaos_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);

    Sequencer sequence_;
    KastleRungler chaos_rungler_;
    Portamento companion_slew_;
    AdsrEnv contour_env_;
    BeatDetector follow_beat_detector_;
    EnvelopeFollower follow_env_follower_;
    EnvelopeFollower follow_transient_follower_;
    SvfStereo filter_;
    StereoDelay follow_delay_{17640};
    StereoDelay chaos_delay_{22050};

    uint32_t current_cv_out_ = 0;
    uint32_t companion_target_ = 0;
    uint32_t companion_out_ = 0;
    uint32_t chaos_shadow_cv_ = 0;
    uint32_t chaos_spread_cv_ = 0;
    size_t step_index_ = 0;
    size_t chaos_step_counter_ = 0;

    bool current_step_active_ = false;
    bool current_step_accent_ = false;
    bool gate_state_ = false;
    bool sync_state_ = false;
    bool chaos_step_burst_ = false;
    bool midi_clock_running_ = false;
    bool midi_clock_event_pending_ = false;
    bool midi_follow_event_pending_ = false;

    uint32_t internal_clock_samples_ = 0;
    uint32_t internal_clock_period_samples_ = 0;
    uint32_t samples_since_last_step_ = 0;
    uint32_t last_step_period_samples_ = 0;
    int32_t external_clock_timeout_samples_ = 0;
    int32_t gate_samples_remaining_ = 0;
    int32_t sync_samples_remaining_ = 0;
    int32_t mode_gate_length_samples_ = 0;
    int32_t chaos_burst_steps_remaining_ = 0;
    int32_t chaos_reseed_countdown_ = 0;

    int32_t rate_value_ = POT_HALF;
    int32_t response_value_ = POT_HALF;
    int32_t range_value_ = POT_TWO_THIRDS;
    int32_t density_value_ = POT_TWO_THIRDS;

    q15_t contour_amount_ = q15(0.65f);
    q15_t dry_wet_ = q15(0.55f);
    q15_t follow_env_value_ = 0;
    q15_t follow_transient_value_ = 0;
    q15_t follow_beat_value_ = 0;
    q15_t follow_threshold_ = q15(0.25f);
    q15_t chaos_motion_ = 0;
    q15_t output_link_amount_ = 0;
    float current_filter_hz_ = 1000.0f;
    EnumArray<Pot, bool> pot_save_pending_ = {};
    EnumArray<Pot, uint32_t> pot_save_timers_ = {};
    Hardware::FeedValue feed_clock_policy_ = Hardware::FeedValue::UNCONNECTED;
    Hardware::FeedValue feed_link_policy_ = Hardware::FeedValue::UNCONNECTED;
    Hardware::FeedValue feed_route_policy_ = Hardware::FeedValue::UNCONNECTED;
    ClockDefault clock_default_ = ClockDefault::AUTO;
    LinkDefault link_default_ = LinkDefault::FREE;
    RouteDefault route_default_ = RouteDefault::NORMAL;
    MidiRole midi_role_ = MidiRole::HYBRID;
    BiasRoute bias_route_ = BiasRoute::SPLIT;
    Personality personality_ = Personality::BALANCED;
    int32_t pitch_main_bias_ = 0;
    int32_t pitch_companion_bias_ = 0;
    int32_t event_bias_samples_ = 0;
    int32_t midi_note_cv_ = 0;
    int32_t midi_velocity_cv_ = 0;
    int32_t midi_note_bias_ = 0;
    int32_t midi_velocity_bias_ = 0;
    uint32_t midi_clock_counter_ = 0;
    uint32_t midi_clock_divider_ = 6;
    uint8_t active_scene_ = 0;
    bool mode_shift_store_armed_ = false;
    bool suppress_mode_tap_ = false;
    size_t mode_shift_hold_ticks_ = 0;
    size_t scene_feedback_ticks_ = 0;
    bool scene_feedback_store_ = false;
    bool scene_morph_active_ = false;
    bool scene_morph_persist_ = false;
    size_t scene_morph_ticks_total_ = 0;
    size_t scene_morph_ticks_elapsed_ = 0;
    EnumArray<Pot, int32_t> scene_morph_start_values_ = {};
    EnumArray<Pot, int32_t> scene_morph_target_values_ = {};
    EnumArray<Pot, bool> scene_override_active_ = {};
    EnumArray<Pot, int32_t> scene_override_values_ = {};

    void InitPots();
    size_t GetSceneAddress(uint8_t slot, size_t offset) const;
    int32_t GetEffectivePotValue(Pot pot) const;
    int32_t GetEffectiveMappedValue(Pot pot) const;
    void StoreScene(uint8_t slot);
    void RecallScene(uint8_t slot, bool persist = true, bool morph = true);
    void UpdateSceneMorph();
    void UpdateSceneOverrides();
    void ResetClockState();
    void ResetForMode(Mode mode);
    void AdvanceStepMode();
    void AdvanceChaosMode();
    void UpdateUiParameters();
    void UpdateAnalogOutputs();
    void UpdatePulseOutputs(bool force = false);
    void HandlePotPersistence();
    bool DetectClockEvent();
    void ProcessStepSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right);
    void ProcessFollowSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right);
    void ProcessChaosSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right);
};

} // namespace kastle2
