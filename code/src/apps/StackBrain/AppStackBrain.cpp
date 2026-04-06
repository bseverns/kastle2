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

#include "AppStackBrain.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include "common/core/Kastle2.hpp"
#include "common/dsp/utility/TriggerGenerator.hpp"
#include "common/utils.hpp"

using namespace kastle2;

namespace
{

constexpr uint32_t kDefaultSequenceLength = 8;
constexpr uint32_t kDefaultStepPeriodSamples = static_cast<uint32_t>(SAMPLE_RATE / 3.0f);
constexpr int32_t kExternalClockHoldSamples = static_cast<int32_t>(SAMPLE_RATE * 1.5f);
constexpr int32_t kMinPulseSamples = 32;
constexpr int32_t kSyncPulseSamples = static_cast<int32_t>(SAMPLE_RATE * 0.004f);
constexpr int32_t kMaxGateSamples = static_cast<int32_t>(SAMPLE_RATE * 0.05f);
constexpr float kMinInternalRateHz = 0.35f;
constexpr float kMaxInternalRateHz = 14.0f;
constexpr float kMinFilterHz = 120.0f;
constexpr float kMaxFilterHz = 6200.0f;
constexpr size_t kMinFollowDelaySamples = static_cast<size_t>(SAMPLE_RATE * 0.018f);
constexpr size_t kMaxFollowDelaySamples = static_cast<size_t>(SAMPLE_RATE * 0.14f);
constexpr size_t kMinChaosDelaySamples = static_cast<size_t>(SAMPLE_RATE * 0.012f);
constexpr size_t kMaxChaosDelaySamples = static_cast<size_t>(SAMPLE_RATE * 0.16f);
constexpr size_t kMaxChaosDelaySpreadSamples = static_cast<size_t>(SAMPLE_RATE * 0.045f);
constexpr uint8_t kMidiCcRateMod = 20;
constexpr uint8_t kMidiCcShapeMod = 21;
constexpr uint8_t kMidiCcSlew = 22;
constexpr uint8_t kMidiCcResponse = 23;
constexpr uint8_t kMidiCcRate = 24;
constexpr uint8_t kMidiCcRange = 25;
constexpr uint8_t kMidiCcDensity = 26;
constexpr uint8_t kMidiCcPitch1Depth = 52;
constexpr uint8_t kMidiCcPitch2Depth = 53;
constexpr uint8_t kMidiCcLink = 54;
constexpr uint8_t kMidiCcEventBias = 55;
constexpr uint8_t kMidiCcMidiNoteDepth = 56;
constexpr uint8_t kMidiCcMidiVelocityDepth = 57;
constexpr uint8_t kMidiCcMidiClockDiv = 58;
constexpr uint8_t kMidiCcModeSelect = 60;
constexpr uint8_t kMidiCcClockDefault = 61;
constexpr uint8_t kMidiCcLinkDefault = 62;
constexpr uint8_t kMidiCcRouteDefault = 63;
constexpr uint8_t kMidiCcMidiRole = 64;
constexpr uint8_t kMidiCcBiasRoute = 65;
constexpr uint8_t kMidiCcPersonality = 66;
constexpr uint8_t kMidiCcSceneSelect = 67;
constexpr uint8_t kMidiCcSceneRecall = 68;
constexpr uint8_t kMidiCcSceneStore = 69;
constexpr std::array<uint32_t, 4> kSceneColors = {
    WS2812::GREEN,
    WS2812::BLUE,
    WS2812::ORANGE,
    WS2812::RED,
};

TriggerGenerator::Type GetTriggerPatternType(const int32_t shape)
{
    if (shape < POT_THIRD)
    {
        return TriggerGenerator::Type::DIVIDER;
    }
    if (shape < POT_TWO_THIRDS)
    {
        return TriggerGenerator::Type::EUCLID;
    }
    return TriggerGenerator::Type::RANDOM;
}

Sequencer::Feed GetCvFeed(const int32_t shape)
{
    if (shape < POT_THIRD)
    {
        return Sequencer::Feed::SAME;
    }
    if (shape < POT_TWO_THIRDS)
    {
        return Sequencer::Feed::INVERT;
    }
    return Sequencer::Feed::RANDOM;
}

KastleRungler::BitIn GetChaosBitIn(const int32_t shape)
{
    if (shape < POT_THIRD)
    {
        return KastleRungler::SAME;
    }
    if (shape < POT_TWO_THIRDS)
    {
        return KastleRungler::INVERT;
    }
    return KastleRungler::RANDOM;
}

q15_t MixQ15(const q15_t dry, const q15_t wet, const q15_t wet_amount)
{
    q15_t dry_amount = q15_inv(wet_amount);
    return q15_add(q15_mult(dry, dry_amount), q15_mult(wet, wet_amount));
}

q15_t ScaleByDac(const uint32_t value)
{
    return pot_to_q15(constrain<int32_t>(value * 4, POT_MIN, POT_MAX));
}

uint8_t SignalBrightness(q15_t signal, uint8_t min = 48, uint8_t max = 255)
{
    return constrain(map(q15_abs(signal), 0, Q15_MAX, min, max), min, max);
}

} // namespace

void AppStackBrain::Init()
{
    inited_ = false;

    Kastle2::base.SetFeatureEnabled(Base::Feature::AUDIO_CHAIN, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::ENV_OUT, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::CV_OUT, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::GATE_OUT, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::SYNC_OUT, false);

    sequence_.Init(kDefaultSequenceLength);
    sequence_.SetLength(kDefaultSequenceLength);
    chaos_rungler_.Init();

    companion_slew_.Init(AUDIO_LOOP_RATE);
    companion_slew_.SetSpeed(0.12f);

    contour_env_.Init(SAMPLE_RATE);
    contour_env_.SetAttackTime(0.002f);
    contour_env_.SetHoldTime(0.0f);
    contour_env_.SetDecayTime(0.12f);
    contour_env_.SetSustainLevel(0);
    contour_env_.SetNonResetting(AdsrEnv::NonResetting::DECAY);

    follow_beat_detector_.Init(SAMPLE_RATE);
    follow_env_follower_.Init(SAMPLE_RATE);
    follow_env_follower_.SetAttackTime(0.01f);
    follow_env_follower_.SetReleaseTime(0.18f);
    follow_transient_follower_.Init(SAMPLE_RATE);
    follow_transient_follower_.SetAttackTime(0.002f);
    follow_transient_follower_.SetReleaseTime(0.03f);

    filter_.Init(SAMPLE_RATE);
    filter_.SetType(SvfStereo::Type::LOWPASS);
    filter_.SetDrive(0.15f);
    filter_.SetResonance(0.18f);
    filter_.SetFrequency(current_filter_hz_);

    follow_delay_.Init(SAMPLE_RATE);
    follow_delay_.SetFilterEnabled(true);
    follow_delay_.SetFeedback(q15(0.3f));
    follow_delay_.SetWet(q15(0.45f));
    follow_delay_.SetFilterResonance(0.18f);
    follow_delay_.SetFilterCrossfade(q15(-0.25f));

    chaos_delay_.Init(SAMPLE_RATE);
    chaos_delay_.SetFilterEnabled(true);
    chaos_delay_.SetFeedback(q15(0.42f));
    chaos_delay_.SetWet(q15(0.55f));
    chaos_delay_.SetFilterResonance(0.28f);
    chaos_delay_.SetFilterCrossfade(q15(-0.1f));

    internal_clock_period_samples_ = kDefaultStepPeriodSamples;
    last_step_period_samples_ = kDefaultStepPeriodSamples;
    mode_gate_length_samples_ = kMinPulseSamples;

    InitPots();

    uint8_t stored_mode = 0;
    if (Kastle2::memory.Read8(kMemMode, &stored_mode) && stored_mode < std::to_underlying(Mode::COUNT))
    {
        current_mode_ = static_cast<Mode>(stored_mode);
    }
    Kastle2::memory.Read8(kMemActiveScene, &active_scene_);
    active_scene_ = constrain<uint8_t>(active_scene_, 0, kSceneCount - 1);

    ResetForMode(current_mode_);
    RecallScene(active_scene_, false, false);
    UpdateUiParameters();
    UpdateAnalogOutputs();
    UpdatePulseOutputs(true);

    inited_ = true;
}

void AppStackBrain::DeInit()
{
    inited_ = false;
    Kastle2::hw.SetAnalogOut(Hardware::AnalogOutput::CV_OUT, 0);
    Kastle2::hw.SetAnalogOut(Hardware::AnalogOutput::ENV_OUT, 0);
    Kastle2::hw.SetGateOut(false);
    Kastle2::hw.SetSyncOut(false);
}

void AppStackBrain::InitPots()
{
    pots_[Pot::RATE_MOD] = FancyPot::Create({
        .pot = Hardware::Pot::POT_1,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcRateMod,
        .deadzone = true,
        .memory_addr = kMemRateMod,
    });

    pots_[Pot::SHAPE_MOD] = FancyPot::Create({
        .pot = Hardware::Pot::POT_2,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcShapeMod,
        .deadzone = true,
        .memory_addr = kMemShapeMod,
    });

    pots_[Pot::SLEW] = FancyPot::Create({
        .pot = Hardware::Pot::POT_3,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_THIRD,
        .midi_cc = kMidiCcSlew,
        .memory_addr = kMemSlew,
    });

    pots_[Pot::RESPONSE] = FancyPot::Create({
        .pot = Hardware::Pot::POT_4,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_TWO_THIRDS,
        .midi_cc = kMidiCcResponse,
        .memory_addr = kMemResponse,
    });

    pots_[Pot::RATE] = FancyPot::Create({
        .pot = Hardware::Pot::POT_5,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcRate,
        .freeze = true,
        .memory_addr = kMemRate,
    });

    pots_[Pot::RANGE] = FancyPot::Create({
        .pot = Hardware::Pot::POT_6,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_TWO_THIRDS,
        .midi_cc = kMidiCcRange,
        .memory_addr = kMemRange,
    });

    pots_[Pot::DENSITY] = FancyPot::Create({
        .pot = Hardware::Pot::POT_7,
        .layer = Hardware::Layer::NORMAL,
        .initial_value = POT_TWO_THIRDS,
        .midi_cc = kMidiCcDensity,
        .memory_addr = kMemDensity,
    });

    pots_[Pot::PITCH_1_DEPTH] = FancyPot::Create({
        .pot = Hardware::Pot::POT_1,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcPitch1Depth,
        .deadzone = true,
    });

    pots_[Pot::PITCH_2_DEPTH] = FancyPot::Create({
        .pot = Hardware::Pot::POT_2,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcPitch2Depth,
        .deadzone = true,
    });

    pots_[Pot::LINK] = FancyPot::Create({
        .pot = Hardware::Pot::POT_3,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_THIRD,
        .midi_cc = kMidiCcLink,
    });

    pots_[Pot::EVENT_BIAS] = FancyPot::Create({
        .pot = Hardware::Pot::POT_4,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcEventBias,
        .deadzone = true,
    });

    pots_[Pot::MIDI_NOTE_DEPTH] = FancyPot::Create({
        .pot = Hardware::Pot::POT_5,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcMidiNoteDepth,
        .deadzone = true,
    });

    pots_[Pot::MIDI_VELOCITY_DEPTH] = FancyPot::Create({
        .pot = Hardware::Pot::POT_6,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_HALF,
        .midi_cc = kMidiCcMidiVelocityDepth,
        .deadzone = true,
    });

    pots_[Pot::MIDI_CLOCK_DIV] = FancyPot::Create({
        .pot = Hardware::Pot::POT_7,
        .layer = Hardware::Layer::SHIFT,
        .initial_value = POT_THIRD,
        .map_size = 6,
        .midi_cc = kMidiCcMidiClockDiv,
    });

    pots_[Pot::MODE_SELECT] = FancyPot::Create({
        .pot = Hardware::Pot::POT_1,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_MIN,
        .map_size = static_cast<int32_t>(Mode::COUNT),
        .midi_cc = kMidiCcModeSelect,
    });

    pots_[Pot::CLOCK_DEFAULT] = FancyPot::Create({
        .pot = Hardware::Pot::POT_2,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_HALF,
        .map_size = static_cast<int32_t>(ClockDefault::COUNT),
        .midi_cc = kMidiCcClockDefault,
        .memory_addr = kMemClockDefault,
    });

    pots_[Pot::LINK_DEFAULT] = FancyPot::Create({
        .pot = Hardware::Pot::POT_3,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_HALF,
        .map_size = static_cast<int32_t>(LinkDefault::COUNT),
        .midi_cc = kMidiCcLinkDefault,
        .memory_addr = kMemLinkDefault,
    });

    pots_[Pot::ROUTE_DEFAULT] = FancyPot::Create({
        .pot = Hardware::Pot::POT_4,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_HALF,
        .map_size = static_cast<int32_t>(RouteDefault::COUNT),
        .midi_cc = kMidiCcRouteDefault,
        .memory_addr = kMemRouteDefault,
    });

    pots_[Pot::MIDI_ROLE] = FancyPot::Create({
        .pot = Hardware::Pot::POT_5,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_MAX,
        .map_size = static_cast<int32_t>(MidiRole::COUNT),
        .midi_cc = kMidiCcMidiRole,
        .memory_addr = kMemMidiRole,
    });

    pots_[Pot::BIAS_ROUTE] = FancyPot::Create({
        .pot = Hardware::Pot::POT_6,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_MIN,
        .map_size = static_cast<int32_t>(BiasRoute::COUNT),
        .midi_cc = kMidiCcBiasRoute,
        .memory_addr = kMemBiasRoute,
    });

    pots_[Pot::PERSONALITY] = FancyPot::Create({
        .pot = Hardware::Pot::POT_7,
        .layer = Hardware::Layer::MODE,
        .initial_value = POT_HALF,
        .map_size = static_cast<int32_t>(Personality::COUNT),
        .midi_cc = kMidiCcPersonality,
        .memory_addr = kMemPersonality,
    });

    for (auto &pot : pots_)
    {
        pot->Init(AUDIO_LOOP_RATE);
    }
}

size_t AppStackBrain::GetSceneAddress(uint8_t slot, size_t offset) const
{
    return kMemSceneBase + (static_cast<size_t>(slot) * kSceneStride) + offset;
}

int32_t AppStackBrain::GetEffectivePotValue(Pot pot) const
{
    int32_t base_value = pots_[pot]->GetValue();
    if (!scene_morph_active_ || pot == Pot::MODE_SELECT)
    {
        if (pot != Pot::MODE_SELECT && scene_override_active_[pot])
        {
            return scene_override_values_[pot];
        }
        return base_value;
    }

    if (scene_morph_ticks_total_ == 0 || scene_morph_ticks_elapsed_ >= scene_morph_ticks_total_)
    {
        return scene_morph_target_values_[pot];
    }

    float morph = static_cast<float>(scene_morph_ticks_elapsed_) / static_cast<float>(scene_morph_ticks_total_);
    return constrain<int32_t>(static_cast<int32_t>(std::lerp(static_cast<float>(scene_morph_start_values_[pot]),
                                                             static_cast<float>(scene_morph_target_values_[pot]),
                                                             morph)),
                              POT_MIN,
                              POT_MAX);
}

int32_t AppStackBrain::GetEffectiveMappedValue(Pot pot) const
{
    size_t map_size = 0;
    switch (pot)
    {
    case Pot::MIDI_CLOCK_DIV:
        map_size = 6;
        break;
    case Pot::MODE_SELECT:
        map_size = static_cast<size_t>(Mode::COUNT);
        break;
    case Pot::CLOCK_DEFAULT:
        map_size = static_cast<size_t>(ClockDefault::COUNT);
        break;
    case Pot::LINK_DEFAULT:
        map_size = static_cast<size_t>(LinkDefault::COUNT);
        break;
    case Pot::ROUTE_DEFAULT:
        map_size = static_cast<size_t>(RouteDefault::COUNT);
        break;
    case Pot::MIDI_ROLE:
        map_size = static_cast<size_t>(MidiRole::COUNT);
        break;
    case Pot::BIAS_ROUTE:
        map_size = static_cast<size_t>(BiasRoute::COUNT);
        break;
    case Pot::PERSONALITY:
        map_size = static_cast<size_t>(Personality::COUNT);
        break;
    case Pot::COUNT:
        break;
    default:
        return 0;
    }

    return constrain<int32_t>(map(GetEffectivePotValue(pot), POT_MIN, POT_MAX, 0, static_cast<int32_t>(map_size) - 1),
                              0,
                              static_cast<int32_t>(map_size) - 1);
}

void AppStackBrain::StoreScene(uint8_t slot)
{
    slot = constrain<uint8_t>(slot, 0, kSceneCount - 1);
    active_scene_ = slot;
    Kastle2::memory.QueueUpdate8(kMemActiveScene, active_scene_);
    Kastle2::memory.QueueUpdate8(GetSceneAddress(slot, 0), std::to_underlying(current_mode_));

    size_t offset = 1;
    for (auto pot_id : EnumRange<Pot>())
    {
        if (pot_id == Pot::MODE_SELECT)
        {
            continue;
        }

        Kastle2::memory.QueueUpdate8(GetSceneAddress(slot, offset), pot_to_mem(GetEffectivePotValue(pot_id)));
        offset++;
    }

    scene_feedback_ticks_ = kSceneFeedbackTicks;
    scene_feedback_store_ = true;
}

void AppStackBrain::RecallScene(uint8_t slot, bool persist, bool morph)
{
    slot = constrain<uint8_t>(slot, 0, kSceneCount - 1);

    uint8_t scene_mode_raw = 0;
    if (!Kastle2::memory.Read8(GetSceneAddress(slot, 0), &scene_mode_raw) || scene_mode_raw >= std::to_underlying(Mode::COUNT))
    {
        return;
    }

    active_scene_ = slot;
    Kastle2::memory.QueueUpdate8(kMemActiveScene, active_scene_);

    EnumArray<Pot, int32_t> recalled_values = {};
    size_t offset = 1;
    for (auto pot_id : EnumRange<Pot>())
    {
        if (pot_id == Pot::MODE_SELECT)
        {
            continue;
        }

        uint8_t stored_value = 0;
        if (Kastle2::memory.Read8(GetSceneAddress(slot, offset), &stored_value))
        {
            recalled_values[pot_id] = mem_to_pot(stored_value);
        }
        else
        {
            recalled_values[pot_id] = GetEffectivePotValue(pot_id);
        }
        offset++;
    }

    Mode target_mode = static_cast<Mode>(scene_mode_raw);
    if (!morph)
    {
        scene_morph_active_ = false;
        for (auto pot_id : EnumRange<Pot>())
        {
            if (pot_id == Pot::MODE_SELECT)
            {
                continue;
            }

            scene_override_active_[pot_id] = true;
            scene_override_values_[pot_id] = recalled_values[pot_id];
            if (persist)
            {
                pots_[pot_id]->ForceValue(recalled_values[pot_id], true);
                pots_[pot_id]->SaveToMemory();
            }
        }

        ResetForMode(target_mode);
        UpdateUiParameters();
        UpdateAnalogOutputs();
        UpdatePulseOutputs(true);
    }
    else
    {
        for (auto pot_id : EnumRange<Pot>())
        {
            scene_morph_start_values_[pot_id] = GetEffectivePotValue(pot_id);
            scene_morph_target_values_[pot_id] = pot_id == Pot::MODE_SELECT ? POT_MIN : recalled_values[pot_id];
        }

        scene_morph_ticks_total_ = kSceneMorphTicks;
        scene_morph_ticks_elapsed_ = 0;
        scene_morph_active_ = true;
        scene_morph_persist_ = persist;

        if (target_mode != current_mode_)
        {
            ResetForMode(target_mode);
        }
    }

    scene_feedback_ticks_ = kSceneFeedbackTicks;
    scene_feedback_store_ = false;
}

void AppStackBrain::UpdateSceneMorph()
{
    if (!scene_morph_active_)
    {
        return;
    }

    if (scene_morph_ticks_elapsed_ < scene_morph_ticks_total_)
    {
        scene_morph_ticks_elapsed_++;
        return;
    }

    for (auto pot_id : EnumRange<Pot>())
    {
        if (pot_id == Pot::MODE_SELECT)
        {
            continue;
        }

        scene_override_active_[pot_id] = true;
        scene_override_values_[pot_id] = scene_morph_target_values_[pot_id];
        if (scene_morph_persist_)
        {
            pots_[pot_id]->ForceValue(scene_morph_target_values_[pot_id], true);
            pots_[pot_id]->SaveToMemory();
        }
    }

    scene_morph_active_ = false;
    scene_morph_persist_ = false;
}

void AppStackBrain::UpdateSceneOverrides()
{
    if (scene_morph_active_)
    {
        return;
    }

    for (auto pot_id : EnumRange<Pot>())
    {
        if (pot_id == Pot::MODE_SELECT || !scene_override_active_[pot_id])
        {
            continue;
        }

        if (diff(pots_[pot_id]->GetValue(), scene_override_values_[pot_id]) <= static_cast<int32_t>(FancyPot::MOVE_THRESHOLD))
        {
            scene_override_active_[pot_id] = false;
        }
    }
}

void AppStackBrain::ResetClockState()
{
    internal_clock_samples_ = 0;
    samples_since_last_step_ = 0;
    last_step_period_samples_ = std::max<uint32_t>(1, internal_clock_period_samples_);
    external_clock_timeout_samples_ = 0;
    gate_samples_remaining_ = 0;
    sync_samples_remaining_ = 0;
    gate_state_ = false;
    sync_state_ = false;
}

void AppStackBrain::ResetForMode(Mode mode)
{
    current_mode_ = mode;
    Kastle2::memory.QueueUpdate8(kMemMode, std::to_underlying(current_mode_));
    if (pots_[Pot::MODE_SELECT])
    {
        pots_[Pot::MODE_SELECT]->ForceValue(
            map(std::to_underlying(current_mode_), 0, std::to_underlying(Mode::COUNT) - 1, POT_MIN, POT_MAX));
    }
    ResetClockState();

    trigger_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    reset_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    follow_audio_gate_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    follow_trigger_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    follow_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    step_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    chaos_sync_edge_ = EdgeDetector(EdgeDetector::Type::RISING);

    contour_env_.Reset();
    follow_beat_detector_.Init(SAMPLE_RATE);
    follow_env_follower_.Init(SAMPLE_RATE);
    follow_transient_follower_.Init(SAMPLE_RATE);
    chaos_rungler_.Init();
    chaos_step_counter_ = 0;

    current_step_active_ = false;
    current_step_accent_ = false;
    chaos_step_burst_ = false;
    midi_clock_running_ = false;
    midi_clock_event_pending_ = false;
    midi_follow_event_pending_ = false;
    step_index_ = 0;
    follow_env_value_ = 0;
    follow_transient_value_ = 0;
    follow_beat_value_ = 0;
    chaos_shadow_cv_ = 0;
    chaos_spread_cv_ = 0;
    chaos_burst_steps_remaining_ = 0;
    chaos_reseed_countdown_ = 0;
    chaos_motion_ = 0;
    output_link_amount_ = 0;
    pitch_main_bias_ = 0;
    pitch_companion_bias_ = 0;
    event_bias_samples_ = 0;
    midi_note_cv_ = 0;
    midi_velocity_cv_ = 0;
    midi_note_bias_ = 0;
    midi_velocity_bias_ = 0;
    midi_clock_counter_ = 0;
    midi_clock_divider_ = 6;
    mode_shift_store_armed_ = false;
    suppress_mode_tap_ = false;
    mode_shift_hold_ticks_ = 0;

    switch (current_mode_)
    {
    case Mode::STEP:
        sequence_.Reset();
        step_index_ = sequence_.GetCurrentStep();
        current_step_active_ = sequence_.GetTriggerOutput();
        current_step_accent_ = current_step_active_;
        current_cv_out_ = sequence_.GetCvOutput();
        companion_target_ = DAC_MAX - current_cv_out_;
        break;
    case Mode::FOLLOW:
        current_cv_out_ = 0;
        companion_target_ = 0;
        break;
    case Mode::CHAOS:
        current_cv_out_ = chaos_rungler_.Step(KastleRungler::RANDOM);
        chaos_shadow_cv_ = chaos_rungler_.Step(KastleRungler::INVERT);
        chaos_spread_cv_ = current_cv_out_ > chaos_shadow_cv_ ? current_cv_out_ - chaos_shadow_cv_ : chaos_shadow_cv_ - current_cv_out_;
        companion_target_ = chaos_shadow_cv_;
        break;
    case Mode::COUNT:
        break;
    }

    companion_out_ = companion_target_;
    UpdateAnalogOutputs();
    UpdatePulseOutputs(true);
}

void AppStackBrain::AdvanceStepMode()
{
    sequence_.NextStep(Sequencer::Feed::SAME, GetCvFeed(response_value_));

    step_index_ = sequence_.GetCurrentStep();
    current_step_active_ = sequence_.GetTriggerOutput();
    current_cv_out_ = sequence_.GetCvOutput();
    current_step_accent_ = current_step_active_ && ((step_index_ % 4) == 0 || current_cv_out_ > (DAC_MAX * 3 / 4));

    current_cv_out_ = (current_cv_out_ * range_value_) / POT_MAX;

    int32_t accent_bias = current_step_accent_ ? DAC_MAX / 6 : 0;
    companion_target_ = constrain(DAC_MAX - static_cast<int32_t>(current_cv_out_) + accent_bias, 0, DAC_MAX);

    if (current_step_active_)
    {
        contour_env_.Trigger();
        int32_t step_gate = static_cast<int32_t>(last_step_period_samples_ / 5);
        step_gate = (step_gate + mode_gate_length_samples_) / 2;
        gate_samples_remaining_ = constrain(step_gate + event_bias_samples_, kMinPulseSamples, kMaxGateSamples);
    }
    else
    {
        gate_samples_remaining_ = 0;
        contour_env_.Reset();
    }

    current_filter_hz_ = std::lerp(kMinFilterHz, kMaxFilterHz, static_cast<float>(current_cv_out_) / static_cast<float>(DAC_MAX));
    filter_.SetFrequency(current_filter_hz_);

    sync_samples_remaining_ = kSyncPulseSamples;
}

void AppStackBrain::AdvanceChaosMode()
{
    uint32_t previous_cv = current_cv_out_;
    bool reseeded = false;
    int32_t personality_index = std::to_underlying(personality_);

    KastleRungler::BitIn primary_bit = GetChaosBitIn(response_value_);
    KastleRungler::BitIn shadow_bit = density_value_ > POT_TWO_THIRDS
                                          ? KastleRungler::RANDOM
                                          : (response_value_ > POT_HALF ? KastleRungler::INVERT : KastleRungler::SAME);

    uint32_t raw_primary = chaos_rungler_.Step(primary_bit);
    if (response_value_ > POT_TWO_THIRDS)
    {
        int32_t reseed_chance = constrain(map(response_value_, POT_TWO_THIRDS, POT_MAX, POT_RANGE / 64, POT_RANGE / 12) + personality_index * (POT_RANGE / 80), 0, POT_RANGE - 1);
        if ((rand() % POT_RANGE) < reseed_chance)
        {
            raw_primary = chaos_rungler_.Step(KastleRungler::RANDOM);
            chaos_reseed_countdown_ = 2;
            reseeded = true;
        }
    }
    if (!reseeded && chaos_reseed_countdown_ > 0)
    {
        chaos_reseed_countdown_--;
    }

    uint32_t raw_shadow = chaos_rungler_.Step(shadow_bit);
    current_cv_out_ = (raw_primary * range_value_) / POT_MAX;
    chaos_shadow_cv_ = (raw_shadow * range_value_) / POT_MAX;
    chaos_spread_cv_ = current_cv_out_ > chaos_shadow_cv_ ? current_cv_out_ - chaos_shadow_cv_ : chaos_shadow_cv_ - current_cv_out_;

    uint32_t motion_delta = current_cv_out_ > previous_cv ? current_cv_out_ - previous_cv : previous_cv - current_cv_out_;
    uint32_t motion_span = std::min<uint32_t>(DAC_MAX, motion_delta + chaos_spread_cv_ / 2);
    chaos_motion_ = pot_to_q15(constrain(map(static_cast<int32_t>(motion_span), 0, DAC_MAX, 0, POT_MAX), POT_MIN, POT_MAX));
    chaos_step_counter_++;

    bool spread_hot = chaos_spread_cv_ > (DAC_MAX / 3);
    bool motion_hot = motion_delta > (DAC_MAX / 4);

    if (chaos_burst_steps_remaining_ <= 0 && density_value_ > POT_HALF)
    {
        int32_t burst_chance = constrain(
            map(density_value_, POT_HALF, POT_MAX, POT_RANGE / 64, POT_RANGE / 10) +
                map(static_cast<int32_t>(chaos_spread_cv_), 0, DAC_MAX, 0, POT_RANGE / 14) +
                personality_index * (POT_RANGE / 80),
            0,
            POT_RANGE - 1);

        if ((rand() % POT_RANGE) < burst_chance)
        {
            chaos_burst_steps_remaining_ = 1 + (chaos_spread_cv_ > (DAC_MAX / 2) ? 1 : 0) + (response_value_ > POT_TWO_THIRDS ? 1 : 0);
        }
    }

    bool in_burst = chaos_burst_steps_remaining_ > 0;
    chaos_step_burst_ = in_burst;
    if (in_burst)
    {
        chaos_burst_steps_remaining_--;
    }

    int32_t active_threshold = constrain(
        density_value_ + map(static_cast<int32_t>(motion_delta), 0, DAC_MAX, -POT_RANGE / 10, POT_RANGE / 5),
        POT_MIN,
        POT_MAX);

    current_step_active_ = in_burst || motion_hot || ((rand() % POT_RANGE) < active_threshold);
    current_step_accent_ = current_step_active_ && (spread_hot || motion_hot || reseeded || chaos_step_burst_ || (chaos_step_counter_ % 7) == 0);

    int32_t spread_bias = map(static_cast<int32_t>(chaos_spread_cv_), 0, DAC_MAX, 0, DAC_MAX / 3);
    int32_t accent_bias = current_step_accent_ ? DAC_MAX / 8 : 0;
    companion_target_ = constrain(static_cast<int32_t>(chaos_shadow_cv_) + spread_bias / 2 + accent_bias, 0, DAC_MAX);

    if (current_step_active_)
    {
        contour_env_.Trigger();
        gate_samples_remaining_ = constrain(mode_gate_length_samples_ + spread_bias / 16 + (current_step_accent_ ? kMinPulseSamples * 2 : 0) + event_bias_samples_, kMinPulseSamples, kMaxGateSamples);
    }
    else
    {
        gate_samples_remaining_ = 0;
        contour_env_.Reset();
    }

    current_filter_hz_ = std::lerp(kMinFilterHz, kMaxFilterHz, static_cast<float>(current_cv_out_) / static_cast<float>(DAC_MAX));
    filter_.SetFrequency(current_filter_hz_);

    size_t delay_center = static_cast<size_t>(std::lerp(static_cast<float>(kMinChaosDelaySamples),
                                                        static_cast<float>(kMaxChaosDelaySamples),
                                                        static_cast<float>(current_cv_out_) / static_cast<float>(DAC_MAX)));
    size_t delay_spread = static_cast<size_t>(std::lerp(0.0f,
                                                        static_cast<float>(kMaxChaosDelaySpreadSamples),
                                                        static_cast<float>(chaos_spread_cv_) / static_cast<float>(DAC_MAX)));
    size_t delay_left = delay_center > delay_spread ? delay_center - delay_spread : kMinChaosDelaySamples;
    size_t delay_right = std::min(kMaxChaosDelaySamples, delay_center + delay_spread);
    chaos_delay_.SetDelay(std::max(kMinChaosDelaySamples, delay_left), std::max(kMinChaosDelaySamples, delay_right));
    sync_samples_remaining_ = kSyncPulseSamples;
}

void AppStackBrain::UpdateUiParameters()
{
    int32_t rate_cv = apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_RATE) - POT_HALF, pots_[Pot::RATE_MOD]->GetValue());
    int32_t shape_cv = apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_SHAPE) - POT_HALF, pots_[Pot::SHAPE_MOD]->GetValue());
    int32_t density_cv = (Kastle2::hw.GetAnalogValue(CV_DENSITY) - POT_HALF) / 2;
    int32_t mode_cv = Kastle2::hw.GetAnalogValue(CV_MODE) - POT_HALF;
    int32_t pitch_main_cv = Kastle2::hw.GetAnalogValue(CV_PITCH_MAIN) - POT_HALF;
    int32_t pitch_companion_cv = Kastle2::hw.GetAnalogValue(CV_PITCH_COMPANION) - POT_HALF;

    rate_value_ = constrain(GetEffectivePotValue(Pot::RATE) + rate_cv, POT_MIN, POT_MAX);
    response_value_ = constrain(GetEffectivePotValue(Pot::RESPONSE) + shape_cv, POT_MIN, POT_MAX);
    density_value_ = constrain(GetEffectivePotValue(Pot::DENSITY) + density_cv, POT_MIN, POT_MAX);
    range_value_ = GetEffectivePotValue(Pot::RANGE);
    int32_t slew_value = GetEffectivePotValue(Pot::SLEW);
    int32_t pitch_1_depth = GetEffectivePotValue(Pot::PITCH_1_DEPTH);
    int32_t pitch_2_depth = GetEffectivePotValue(Pot::PITCH_2_DEPTH);
    int32_t link_value = GetEffectivePotValue(Pot::LINK);
    int32_t event_bias_value = GetEffectivePotValue(Pot::EVENT_BIAS);
    int32_t midi_note_depth = GetEffectivePotValue(Pot::MIDI_NOTE_DEPTH);
    int32_t midi_velocity_depth = GetEffectivePotValue(Pot::MIDI_VELOCITY_DEPTH);
    int32_t midi_clock_div_value = GetEffectiveMappedValue(Pot::MIDI_CLOCK_DIV);
    int32_t mode_select_value = GetEffectiveMappedValue(Pot::MODE_SELECT);
    int32_t clock_default_value = GetEffectiveMappedValue(Pot::CLOCK_DEFAULT);
    int32_t link_default_value = GetEffectiveMappedValue(Pot::LINK_DEFAULT);
    int32_t route_default_value = GetEffectiveMappedValue(Pot::ROUTE_DEFAULT);
    int32_t midi_role_value = GetEffectiveMappedValue(Pot::MIDI_ROLE);
    int32_t bias_route_value = GetEffectiveMappedValue(Pot::BIAS_ROUTE);
    int32_t personality_value = GetEffectiveMappedValue(Pot::PERSONALITY);

    float rate_norm = static_cast<float>(rate_value_) / static_cast<float>(POT_MAX);
    float response_norm = static_cast<float>(response_value_) / static_cast<float>(POT_MAX);
    float density_norm = static_cast<float>(density_value_) / static_cast<float>(POT_MAX);
    float slew_norm = static_cast<float>(slew_value) / static_cast<float>(POT_MAX);
    float tonal_norm = static_cast<float>(current_cv_out_ + companion_out_) / static_cast<float>(DAC_MAX * 2);

    feed_clock_policy_ = Kastle2::hw.GetFeedValue(FEED_CLOCK);
    feed_link_policy_ = Kastle2::hw.GetFeedValue(FEED_LINK);
    feed_route_policy_ = Kastle2::hw.GetFeedValue(FEED_ROUTE);
    clock_default_ = static_cast<ClockDefault>(constrain<int32_t>(clock_default_value, 0, std::to_underlying(ClockDefault::COUNT) - 1));
    link_default_ = static_cast<LinkDefault>(constrain<int32_t>(link_default_value, 0, std::to_underlying(LinkDefault::COUNT) - 1));
    route_default_ = static_cast<RouteDefault>(constrain<int32_t>(route_default_value, 0, std::to_underlying(RouteDefault::COUNT) - 1));
    midi_role_ = static_cast<MidiRole>(constrain<int32_t>(midi_role_value, 0, std::to_underlying(MidiRole::COUNT) - 1));
    bias_route_ = static_cast<BiasRoute>(constrain<int32_t>(bias_route_value, 0, std::to_underlying(BiasRoute::COUNT) - 1));
    personality_ = static_cast<Personality>(constrain<int32_t>(personality_value, 0, std::to_underlying(Personality::COUNT) - 1));

    pitch_main_bias_ = apply_pot_mod_attenuvert(pitch_main_cv, pitch_1_depth) / 4;
    pitch_companion_bias_ = apply_pot_mod_attenuvert(pitch_companion_cv, pitch_2_depth) / 4;
    output_link_amount_ = pot_to_q15(link_value);
    event_bias_samples_ = map(event_bias_value, POT_MIN, POT_MAX, -kMinPulseSamples * 2, kMinPulseSamples * 3);
    midi_note_bias_ = apply_pot_mod_attenuvert(midi_note_cv_, midi_note_depth) / 4;
    midi_velocity_bias_ = apply_pot_mod_attenuvert(midi_velocity_cv_, midi_velocity_depth) / 4;

    static constexpr std::array<uint32_t, 6> midi_division_table = {24, 12, 6, 3, 2, 1};
    midi_clock_divider_ = midi_division_table[constrain<int32_t>(midi_clock_div_value, 0, static_cast<int32_t>(midi_division_table.size() - 1))];

    if (Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        Mode selected_mode = static_cast<Mode>(constrain<int32_t>(mode_select_value, 0, std::to_underlying(Mode::COUNT) - 1));
        if (selected_mode != current_mode_)
        {
            ResetForMode(selected_mode);
        }
    }

    switch (current_mode_)
    {
    case Mode::STEP:
    {
        response_value_ = constrain(response_value_ + mode_cv / 2, POT_MIN, POT_MAX);
        density_value_ = constrain(density_value_ + mode_cv / 3, POT_MIN, POT_MAX);
        float personality_norm = static_cast<float>(std::to_underlying(personality_)) / static_cast<float>(std::to_underlying(Personality::COUNT) - 1);
        internal_clock_period_samples_ = std::max<uint32_t>(
            1,
            static_cast<uint32_t>(SAMPLE_RATE / std::lerp(kMinInternalRateHz, kMaxInternalRateHz, rate_norm)));

        size_t sequence_length = response_value_ > POT_TWO_THIRDS ? 16 : (response_value_ > POT_THIRD ? 12 : 8);
        if (sequence_.GetLength() != sequence_length)
        {
            sequence_.SetLength(sequence_length);
            step_index_ = sequence_.GetCurrentStep();
            current_step_active_ = sequence_.GetTriggerOutput();
            current_cv_out_ = sequence_.GetCvOutput();
        }
        sequence_.GenerateTriggers(GetTriggerPatternType(response_value_), pot_to_q15(density_value_));

        contour_amount_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_THIRD, POT_MAX), POT_MIN, POT_MAX));
        dry_wet_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_HALF / 2, POT_MAX), POT_MIN, POT_MAX));
        companion_slew_.SetSpeed(std::lerp(0.01f, 0.35f, slew_norm));
        current_filter_hz_ = std::lerp(kMinFilterHz, kMaxFilterHz, tonal_norm);
        filter_.SetFrequency(current_filter_hz_);
        filter_.SetResonance(std::lerp(0.08f, 0.72f + 0.12f * personality_norm, response_norm));
        filter_.SetDrive(std::lerp(0.05f, 0.45f + 0.12f * personality_norm, response_norm));
        mode_gate_length_samples_ = constrain<int32_t>(static_cast<int32_t>(std::lerp(0.012f, 0.08f, personality_norm) * SAMPLE_RATE), kMinPulseSamples, kMaxGateSamples);
        break;
    }
    case Mode::FOLLOW:
    {
        float personality_norm = static_cast<float>(std::to_underlying(personality_)) / static_cast<float>(std::to_underlying(Personality::COUNT) - 1);
        float attack = std::lerp(0.08f, 0.001f, rate_norm);
        float release = std::lerp(0.45f, 0.03f, rate_norm);
        float transient_release = std::lerp(0.09f, 0.01f, response_norm);
        int32_t threshold_bias = mode_cv / 3;

        follow_env_follower_.SetAttackTime(attack);
        follow_env_follower_.SetReleaseTime(release);
        follow_transient_follower_.SetAttackTime(0.001f);
        follow_transient_follower_.SetReleaseTime(transient_release);

        int32_t threshold_pot = constrain(map(density_value_, POT_MIN, POT_MAX, static_cast<int32_t>(POT_MAX * 0.55f), static_cast<int32_t>(POT_MAX * 0.08f)) + threshold_bias,
                                          POT_MIN,
                                          POT_MAX);
        follow_threshold_ = pot_to_q15(threshold_pot);
        contour_amount_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_THIRD / 2, POT_MAX), POT_MIN, POT_MAX));
        dry_wet_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_THIRD, POT_MAX), POT_MIN, POT_MAX));
        companion_slew_.SetSpeed(std::lerp(0.005f, 0.25f, slew_norm));
        mode_gate_length_samples_ = constrain<int32_t>(static_cast<int32_t>(std::lerp(0.01f, 0.08f, density_norm) * SAMPLE_RATE), kMinPulseSamples, kMaxGateSamples);

        current_filter_hz_ = std::lerp(kMinFilterHz, kMaxFilterHz, tonal_norm);
        filter_.SetFrequency(current_filter_hz_);
        filter_.SetResonance(std::lerp(0.06f, 0.38f + 0.12f * personality_norm, response_norm));
        filter_.SetDrive(std::lerp(0.02f, 0.28f + 0.12f * personality_norm, response_norm));
        follow_delay_.SetFeedback(float_to_q15(std::lerp(0.08f, 0.55f + 0.12f * personality_norm, response_norm)));
        follow_delay_.SetWet(float_to_q15(std::lerp(0.15f, 0.68f + 0.1f * personality_norm, response_norm)));
        follow_delay_.SetFilterResonance(std::lerp(0.05f, 0.36f, density_norm));
        follow_delay_.SetFilterCrossfade(float_to_q15(std::lerp(-0.55f, 0.1f, response_norm)));
        break;
    }
    case Mode::CHAOS:
    {
        response_value_ = constrain(response_value_ + mode_cv / 2, POT_MIN, POT_MAX);
        density_value_ = constrain(density_value_ + mode_cv / 2, POT_MIN, POT_MAX);
        float personality_norm = static_cast<float>(std::to_underlying(personality_)) / static_cast<float>(std::to_underlying(Personality::COUNT) - 1);
        internal_clock_period_samples_ = std::max<uint32_t>(
            1,
            static_cast<uint32_t>(SAMPLE_RATE / std::lerp(kMinInternalRateHz, kMaxInternalRateHz * 1.4f, rate_norm)));

        contour_amount_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_HALF, POT_MAX), POT_MIN, POT_MAX));
        dry_wet_ = pot_to_q15(constrain(map(response_value_, POT_MIN, POT_MAX, POT_THIRD, POT_MAX), POT_MIN, POT_MAX));
        companion_slew_.SetSpeed(std::lerp(0.02f, 0.45f, slew_norm));
        mode_gate_length_samples_ = constrain<int32_t>(static_cast<int32_t>(std::lerp(0.012f, 0.085f, density_norm) * SAMPLE_RATE), kMinPulseSamples, kMaxGateSamples);
        current_filter_hz_ = std::lerp(kMinFilterHz, kMaxFilterHz, tonal_norm);
        filter_.SetFrequency(current_filter_hz_);
        filter_.SetResonance(std::lerp(0.14f, 0.92f + 0.05f * personality_norm, response_norm));
        filter_.SetDrive(std::lerp(0.12f, 0.75f + 0.1f * personality_norm, density_norm));
        chaos_delay_.SetFeedback(float_to_q15(std::lerp(0.18f, 0.72f + 0.08f * personality_norm, density_norm)));
        chaos_delay_.SetWet(float_to_q15(std::lerp(0.28f, 0.84f + 0.05f * personality_norm, response_norm)));
        chaos_delay_.SetFilterResonance(std::lerp(0.08f, 0.52f, response_norm));
        chaos_delay_.SetFilterCrossfade(float_to_q15(std::lerp(-0.45f, 0.65f, density_norm)));
        break;
    }
    case Mode::COUNT:
        break;
    }
}

void AppStackBrain::UpdateAnalogOutputs()
{
    companion_out_ = constrain<int32_t>(static_cast<int32_t>(companion_slew_.Track(static_cast<float>(companion_target_) / static_cast<float>(DAC_MAX)) * DAC_MAX), 0, DAC_MAX);

    int32_t main_bias = pitch_main_bias_;
    int32_t companion_bias = pitch_companion_bias_;
    bool midi_bias_enabled = midi_role_ == MidiRole::BIAS || midi_role_ == MidiRole::HYBRID;

    if (midi_bias_enabled)
    {
        switch (bias_route_)
        {
        case BiasRoute::SPLIT:
            main_bias += midi_note_bias_;
            companion_bias += midi_velocity_bias_;
            break;
        case BiasRoute::CROSS:
            main_bias += midi_velocity_bias_;
            companion_bias += midi_note_bias_;
            std::swap(main_bias, companion_bias);
            break;
        case BiasRoute::MAIN:
            main_bias += midi_note_bias_ + midi_velocity_bias_;
            companion_bias = 0;
            break;
        case BiasRoute::BOTH:
        {
            int32_t combined = (midi_note_bias_ + midi_velocity_bias_) / 2;
            main_bias += combined;
            companion_bias += combined;
            break;
        }
        case BiasRoute::COUNT:
            break;
        }
    }

    int32_t cv_out = constrain<int32_t>(static_cast<int32_t>(current_cv_out_) + main_bias, 0, DAC_MAX);
    int32_t env_out = constrain<int32_t>(companion_out_ + companion_bias, 0, DAC_MAX);

    Hardware::FeedValue link_policy = feed_link_policy_;
    if (link_policy == Hardware::FeedValue::UNCONNECTED)
    {
        switch (link_default_)
        {
        case LinkDefault::INVERT:
            link_policy = Hardware::FeedValue::LOW;
            break;
        case LinkDefault::MERGE:
            link_policy = Hardware::FeedValue::HIGH;
            break;
        case LinkDefault::FREE:
        case LinkDefault::COUNT:
            link_policy = Hardware::FeedValue::UNCONNECTED;
            break;
        }
    }

    switch (link_policy)
    {
    case Hardware::FeedValue::LOW:
        env_out = DAC_MAX - env_out;
        break;
    case Hardware::FeedValue::HIGH:
        env_out = constrain<int32_t>((env_out + cv_out) / 2, 0, DAC_MAX);
        break;
    case Hardware::FeedValue::UNCONNECTED:
        break;
    }

    if (output_link_amount_ > 0)
    {
        int32_t linked_avg = (cv_out + env_out) / 2;
        cv_out = constrain<int32_t>(cv_out + q15_mult(output_link_amount_, linked_avg - cv_out), 0, DAC_MAX);
        env_out = constrain<int32_t>(env_out + q15_mult(output_link_amount_, linked_avg - env_out), 0, DAC_MAX);
    }

    Hardware::FeedValue route_policy = feed_route_policy_;
    if (route_policy == Hardware::FeedValue::UNCONNECTED)
    {
        switch (route_default_)
        {
        case RouteDefault::SWAP:
            route_policy = Hardware::FeedValue::LOW;
            break;
        case RouteDefault::BLEND:
            route_policy = Hardware::FeedValue::HIGH;
            break;
        case RouteDefault::NORMAL:
        case RouteDefault::COUNT:
            route_policy = Hardware::FeedValue::UNCONNECTED;
            break;
        }
    }

    switch (route_policy)
    {
    case Hardware::FeedValue::LOW:
        std::swap(cv_out, env_out);
        break;
    case Hardware::FeedValue::HIGH:
        env_out = constrain<int32_t>((env_out + cv_out) / 2 + (current_step_accent_ ? DAC_MAX / 10 : 0), 0, DAC_MAX);
        break;
    case Hardware::FeedValue::UNCONNECTED:
        break;
    }

    Kastle2::hw.SetAnalogOut(Hardware::AnalogOutput::CV_OUT, cv_out);
    Kastle2::hw.SetAnalogOut(Hardware::AnalogOutput::ENV_OUT, env_out);
}

void AppStackBrain::UpdatePulseOutputs(bool force)
{
    bool next_gate = gate_samples_remaining_ > 0;
    bool next_sync = sync_samples_remaining_ > 0;

    if (force || next_gate != gate_state_)
    {
        gate_state_ = next_gate;
        Kastle2::hw.SetGateOut(gate_state_);
    }

    if (force || next_sync != sync_state_)
    {
        sync_state_ = next_sync;
        Kastle2::hw.SetSyncOut(sync_state_);
    }
}

void AppStackBrain::HandlePotPersistence()
{
    for (auto pot_id : EnumRange<Pot>())
    {
        auto &pot = pots_[pot_id];
        if (pot->HasMoved(FancyPot::Move::TWEAK))
        {
            pot_save_pending_[pot_id] = true;
            pot_save_timers_[pot_id] = 0;
            continue;
        }

        if (!pot_save_pending_[pot_id])
        {
            continue;
        }

        pot_save_timers_[pot_id]++;
        if (pot_save_timers_[pot_id] >= kPotSaveDebounceTicks)
        {
            pot->SaveToMemory();
            pot_save_pending_[pot_id] = false;
            pot_save_timers_[pot_id] = 0;
        }
    }
}

bool AppStackBrain::DetectClockEvent()
{
    bool trigger_event = trigger_edge_.Process(Kastle2::hw.GetTriggerIn());
    bool midi_event = midi_clock_event_pending_;
    Hardware::FeedValue clock_policy = feed_clock_policy_;
    if (clock_policy == Hardware::FeedValue::UNCONNECTED)
    {
        switch (clock_default_)
        {
        case ClockDefault::TRIGGER:
            clock_policy = Hardware::FeedValue::LOW;
            break;
        case ClockDefault::MIDI:
            clock_policy = Hardware::FeedValue::HIGH;
            break;
        case ClockDefault::AUTO:
        case ClockDefault::COUNT:
            clock_policy = Hardware::FeedValue::UNCONNECTED;
            break;
        }
    }

    if (midi_event)
    {
        midi_clock_event_pending_ = false;
    }

    if (clock_policy == Hardware::FeedValue::HIGH)
    {
        if (midi_event)
        {
            external_clock_timeout_samples_ = kExternalClockHoldSamples;
            return true;
        }
        if (trigger_event)
        {
            external_clock_timeout_samples_ = kExternalClockHoldSamples;
            return true;
        }
    }
    else if (clock_policy == Hardware::FeedValue::LOW)
    {
        if (trigger_event)
        {
            external_clock_timeout_samples_ = kExternalClockHoldSamples;
            return true;
        }
    }
    else
    {
        if (trigger_event)
        {
            external_clock_timeout_samples_ = kExternalClockHoldSamples;
            return true;
        }
        if (midi_event)
        {
            external_clock_timeout_samples_ = kExternalClockHoldSamples;
            return true;
        }
    }

    if (external_clock_timeout_samples_ > 0)
    {
        return false;
    }

    internal_clock_samples_++;
    if (internal_clock_samples_ >= internal_clock_period_samples_)
    {
        internal_clock_samples_ = 0;
        return true;
    }

    return false;
}

void AppStackBrain::ProcessStepSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right)
{
    q15_t contour = q31_to_q15(contour_env_.Process());
    q15_t contour_gain = q15_add(q15(0.12f), q15_mult(contour, contour_amount_));
    if (current_step_accent_)
    {
        contour_gain = q15_saturate(contour_gain + q15(0.2f));
    }

    q15_t wet_left = q15_mult(left, contour_gain);
    q15_t wet_right = q15_mult(right, contour_gain);

    filter_.Process(wet_left, wet_right);
    out_left = MixQ15(left, filter_.GetLeft(), dry_wet_);
    out_right = MixQ15(right, filter_.GetRight(), dry_wet_);
}

void AppStackBrain::ProcessFollowSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right)
{
    int32_t personality_index = std::to_underlying(personality_);
    q15_t mono_peak = std::max(q15_abs(left), q15_abs(right));
    bool trigger_edge = follow_trigger_edge_.Process(Kastle2::hw.GetTriggerIn());
    bool sync_edge = follow_sync_edge_.Process(Kastle2::hw.GetSyncIn());
    bool midi_event = midi_follow_event_pending_;
    if (midi_event)
    {
        midi_follow_event_pending_ = false;
    }

    follow_env_follower_.Track(mono_peak);
    follow_transient_follower_.Track(mono_peak);
    follow_beat_value_ = follow_beat_detector_.AudioProcess(mono_peak) ? Q15_MAX : Q15_ZERO;

    follow_env_value_ = follow_env_follower_.GetEnvelope();
    follow_transient_value_ = follow_transient_follower_.GetEnvelope();
    q15_t intensity = q15_saturate(follow_env_value_ + q15_mult(follow_beat_value_, q15(0.22f)));
    q15_t transient_mix = q15_saturate(follow_transient_value_ + q15_mult(follow_beat_value_, q15(0.38f)));

    current_cv_out_ = (map(q15_abs(intensity), 0, Q15_MAX, 0, DAC_MAX) * range_value_) / POT_MAX;
    companion_target_ = (map(q15_abs(transient_mix), 0, Q15_MAX, 0, DAC_MAX) * range_value_) / POT_MAX;

    bool transient_hot = follow_transient_value_ > follow_threshold_;
    bool audio_event = follow_audio_gate_edge_.Process(transient_hot);
    bool beat_event = follow_beat_value_ > 0;
    bool midi_events_enabled = midi_role_ == MidiRole::EVENTS || midi_role_ == MidiRole::HYBRID;
    bool beat_enabled = personality_ != Personality::TAME;
    bool follow_event = audio_event || trigger_edge || sync_edge;

    if (feed_clock_policy_ == Hardware::FeedValue::UNCONNECTED)
    {
        follow_event = follow_event || (beat_enabled && beat_event) || (midi_events_enabled && midi_event);
    }
    else if (feed_clock_policy_ == Hardware::FeedValue::HIGH)
    {
        follow_event = follow_event || (beat_enabled && beat_event) || (midi_events_enabled && midi_event);
    }

    if (follow_event)
    {
        int32_t accent_extension = ((beat_enabled && beat_event) ? kMinPulseSamples * (2 + personality_index / 2) : 0) +
                                   ((trigger_edge || sync_edge || (midi_events_enabled && midi_event)) ? kMinPulseSamples : 0);
        gate_samples_remaining_ = constrain(mode_gate_length_samples_ + accent_extension + event_bias_samples_, kMinPulseSamples, kMaxGateSamples);
        sync_samples_remaining_ = kSyncPulseSamples;
        current_step_active_ = true;
        current_step_accent_ = trigger_edge || sync_edge || (midi_events_enabled && midi_event) || follow_env_value_ > q15(0.55f) || (beat_enabled && beat_event);
        contour_env_.Trigger();
    }
    else if (gate_samples_remaining_ <= 0)
    {
        current_step_active_ = false;
        current_step_accent_ = false;
    }

    float follow_cutoff = std::lerp(
        kMinFilterHz,
        kMaxFilterHz,
        static_cast<float>(q15_abs(q15_add(intensity, transient_mix))) / static_cast<float>(Q15_MAX));
    filter_.SetFrequency(follow_cutoff);

    size_t delay_center = static_cast<size_t>(std::lerp(static_cast<float>(kMinFollowDelaySamples),
                                                        static_cast<float>(kMaxFollowDelaySamples),
                                                        static_cast<float>(q15_abs(intensity)) / static_cast<float>(Q15_MAX)));
    size_t delay_spread = static_cast<size_t>(std::lerp(0.0f,
                                                        static_cast<float>(kMinFollowDelaySamples / 2),
                                                        static_cast<float>(q15_abs(transient_mix)) / static_cast<float>(Q15_MAX)));
    follow_delay_.SetDelay(std::max(kMinFollowDelaySamples, delay_center),
                           std::min(kMaxFollowDelaySamples, delay_center + delay_spread));

    q15_t contour = q31_to_q15(contour_env_.Process());
    q15_t contour_gain = q15_add(q15(0.06f), q15_mult(transient_mix, contour_amount_));
    contour_gain = q15_add(contour_gain, q15_mult(contour, q15(0.35f)));
    if (current_step_accent_)
    {
        contour_gain = q15_saturate(contour_gain + q15(0.12f));
    }

    q15_t wet_left = q15_mult(left, contour_gain);
    q15_t wet_right = q15_mult(right, contour_gain);
    if (sync_edge)
    {
        wet_right = q15_mult(wet_right, q15(-0.65f));
    }

    filter_.Process(wet_left, wet_right);
    StereoDelay::Output delayed = follow_delay_.Process(filter_.GetLeft(), filter_.GetRight());
    q15_t wet_mix_left = q15_add(q15_mult(filter_.GetLeft(), q15(0.72f)), q15_mult(delayed.left, q15(0.55f)));
    q15_t wet_mix_right = q15_add(q15_mult(filter_.GetRight(), q15(0.72f)), q15_mult(delayed.right, q15(0.55f)));
    out_left = MixQ15(left, wet_mix_left, dry_wet_);
    out_right = MixQ15(right, wet_mix_right, dry_wet_);
}

void AppStackBrain::ProcessChaosSample(q15_t left, q15_t right, q15_t &out_left, q15_t &out_right)
{
    q15_t contour = q31_to_q15(contour_env_.Process());
    q15_t chaos_depth = ScaleByDac(current_cv_out_);
    q15_t shadow_depth = ScaleByDac(chaos_shadow_cv_);
    q15_t spread_depth = ScaleByDac(chaos_spread_cv_);
    q15_t contour_gain = q15_add(q15(0.05f), q15_mult(contour, contour_amount_));
    contour_gain = q15_add(contour_gain, q15_mult(chaos_depth, q15(0.18f)));
    contour_gain = q15_add(contour_gain, q15_mult(spread_depth, q15(0.16f)));
    contour_gain = q15_add(contour_gain, q15_mult(chaos_motion_, q15(0.3f)));
    contour_gain = q15_saturate(contour_gain);

    if (current_step_accent_)
    {
        contour_gain = q15_saturate(contour_gain + q15(0.16f));
    }

    q15_t left_gain = q15_saturate(contour_gain + q15_mult(shadow_depth, q15(0.12f)));
    q15_t right_gain = q15_saturate(contour_gain - q15_mult(spread_depth, q15(0.08f)));
    q15_t wet_left = q15_mult(left, left_gain);
    q15_t wet_right = q15_mult(right, right_gain);

    if (current_step_accent_)
    {
        wet_right = q15_mult(wet_right, q15(-0.72f));
    }
    if (chaos_step_burst_)
    {
        wet_left = q15_saturate(wet_left + q15_mult(wet_left, q15(0.2f)));
    }

    filter_.Process(wet_left, wet_right);
    StereoDelay::Output delayed = chaos_delay_.Process(filter_.GetLeft(), filter_.GetRight());
    q15_t wet_mix_left = q15_add(q15_mult(filter_.GetLeft(), q15(0.55f)), q15_mult(delayed.left, q15(0.75f)));
    q15_t wet_mix_right = q15_add(q15_mult(filter_.GetRight(), q15(0.55f)), q15_mult(delayed.right, q15(0.75f)));
    out_left = MixQ15(left, wet_mix_left, dry_wet_);
    out_right = MixQ15(right, wet_mix_right, dry_wet_);
}

FASTCODE void AppStackBrain::AudioLoop(q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        for (size_t i = 0; i < size * 2; i++)
        {
            output[i] = 0;
        }
        return;
    }

    companion_slew_.TimeTick();

    for (auto &pot : pots_)
    {
        pot->Process();
    }

    for (size_t i = 0; i < size; i++)
    {
        q15_t out_left = input[2 * i];
        q15_t out_right = input[2 * i + 1];

        if (reset_edge_.Process(Kastle2::hw.GetResetIn()))
        {
            ResetForMode(current_mode_);
            UpdateUiParameters();
        }

        if (current_mode_ == Mode::STEP || current_mode_ == Mode::CHAOS)
        {
            if (DetectClockEvent())
            {
                last_step_period_samples_ = std::max<uint32_t>(1, samples_since_last_step_);
                samples_since_last_step_ = 0;

                if (current_mode_ == Mode::STEP)
                {
                    AdvanceStepMode();
                }
                else
                {
                    AdvanceChaosMode();
                }
            }
        }

        if (current_mode_ == Mode::STEP && step_sync_edge_.Process(Kastle2::hw.GetSyncIn()))
        {
            current_step_accent_ = true;
            contour_env_.Trigger();
            companion_target_ = constrain<int32_t>(static_cast<int32_t>(companion_target_) + DAC_MAX / 10, 0, DAC_MAX);
            gate_samples_remaining_ = std::max<int32_t>(gate_samples_remaining_, kMinPulseSamples * 2);
            sync_samples_remaining_ = kSyncPulseSamples;
        }

        if (current_mode_ == Mode::CHAOS && chaos_sync_edge_.Process(Kastle2::hw.GetSyncIn()))
        {
            chaos_burst_steps_remaining_ = std::max<int32_t>(chaos_burst_steps_remaining_, 2 + (response_value_ > POT_TWO_THIRDS ? 1 : 0));
            chaos_reseed_countdown_ = std::max<int32_t>(chaos_reseed_countdown_, 2);
            current_step_active_ = true;
            current_step_accent_ = true;
            contour_env_.Trigger();
            gate_samples_remaining_ = std::max<int32_t>(gate_samples_remaining_, kMinPulseSamples * 3);
            sync_samples_remaining_ = kSyncPulseSamples;
        }

        switch (current_mode_)
        {
        case Mode::STEP:
            ProcessStepSample(input[2 * i], input[2 * i + 1], out_left, out_right);
            break;
        case Mode::FOLLOW:
            ProcessFollowSample(input[2 * i], input[2 * i + 1], out_left, out_right);
            break;
        case Mode::CHAOS:
            ProcessChaosSample(input[2 * i], input[2 * i + 1], out_left, out_right);
            break;
        case Mode::COUNT:
            break;
        }

        output[2 * i] = out_left;
        output[2 * i + 1] = out_right;

        if (gate_samples_remaining_ > 0)
        {
            gate_samples_remaining_--;
        }
        if (sync_samples_remaining_ > 0)
        {
            sync_samples_remaining_--;
        }
        if (external_clock_timeout_samples_ > 0)
        {
            external_clock_timeout_samples_--;
        }
        if (current_mode_ != Mode::FOLLOW)
        {
            samples_since_last_step_++;
        }

        UpdatePulseOutputs();
    }

    UpdateAnalogOutputs();
}

void AppStackBrain::UiLoop()
{
    UpdateSceneMorph();

    for (auto &pot : pots_)
    {
        pot->ReadValue();
    }

    UpdateSceneOverrides();

    if (ShiftTrigger())
    {
        RecallScene((active_scene_ + 1) % kSceneCount);
    }

    if (Kastle2::hw.JustReleased(Hardware::Button::MODE))
    {
        if (suppress_mode_tap_)
        {
            suppress_mode_tap_ = false;
        }
        else if (Kastle2::base.GetPrevLayer() == Hardware::Layer::MODE &&
                 Kastle2::base.GetPrevLayerTimer() < kModeTapTicks)
        {
            ResetForMode(EnumIncrement(current_mode_));
        }
    }

    if (Kastle2::hw.Pressed(Hardware::Button::MODE) && Kastle2::hw.JustPressed(Hardware::Button::SHIFT))
    {
        mode_shift_store_armed_ = true;
        mode_shift_hold_ticks_ = 0;
    }

    if (mode_shift_store_armed_)
    {
        if (Kastle2::hw.Pressed(Hardware::Button::MODE) && Kastle2::hw.Pressed(Hardware::Button::SHIFT))
        {
            mode_shift_hold_ticks_++;
        }
        else if (Kastle2::hw.Pressed(Hardware::Button::MODE) &&
                 Kastle2::hw.JustReleased(Hardware::Button::SHIFT) &&
                 mode_shift_hold_ticks_ < kModeShiftTapTicks &&
                 Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
        {
            StoreScene(active_scene_);
            suppress_mode_tap_ = true;
            mode_shift_store_armed_ = false;
        }
        else
        {
            mode_shift_store_armed_ = false;
        }
    }

    UpdateUiParameters();
    HandlePotPersistence();

    uint32_t led_1 = mode_colors_[current_mode_];
    uint32_t led_2 = current_step_accent_ ? WS2812::ORANGE : (current_step_active_ ? WS2812::WHITE : WS2812::NONE);
    uint32_t led_3 = WS2812::NONE;

    switch (current_mode_)
    {
    case Mode::STEP:
        led_1 = WS2812::ApplyBrightness(mode_colors_[current_mode_], SignalBrightness(q31_to_q15(contour_env_.GetOutput()), 64, 255));
        led_3 = external_clock_timeout_samples_ > 0 ? WS2812::BLUE : WS2812::WHITE;
        break;
    case Mode::FOLLOW:
        led_1 = WS2812::ApplyBrightness(mode_colors_[current_mode_], SignalBrightness(follow_env_value_, 40, 255));
        led_2 = follow_beat_value_ > 0
                    ? WS2812::WHITE
                    : follow_transient_value_ > follow_threshold_
                    ? WS2812::ORANGE
                    : WS2812::ApplyBrightness(WS2812::BLUE, SignalBrightness(follow_transient_value_, 24, 160));
        led_3 = sync_samples_remaining_ > 0
                    ? (follow_beat_value_ > 0 ? WS2812::GREEN : WS2812::WHITE)
                    : WS2812::NONE;
        break;
    case Mode::CHAOS:
        led_1 = WS2812::ApplyBrightness(mode_colors_[current_mode_], constrain(map(current_cv_out_, 0, DAC_MAX, 48, 255), 48, 255));
        led_2 = chaos_step_burst_ ? WS2812::ORANGE : (current_step_accent_ ? WS2812::WHITE : led_2);
        led_3 = chaos_reseed_countdown_ > 0 ? WS2812::RED : (external_clock_timeout_samples_ > 0 ? WS2812::BLUE : WS2812::WHITE);
        break;
    case Mode::COUNT:
        break;
    }

    if (Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        Mode selected_mode = static_cast<Mode>(GetEffectiveMappedValue(Pot::MODE_SELECT));
        led_1 = mode_colors_[selected_mode];

        switch (clock_default_)
        {
        case ClockDefault::TRIGGER:
            led_2 = WS2812::RED;
            break;
        case ClockDefault::MIDI:
            led_2 = WS2812::BLUE;
            break;
        case ClockDefault::AUTO:
        case ClockDefault::COUNT:
            led_2 = WS2812::WHITE;
            break;
        }

        switch (midi_role_)
        {
        case MidiRole::OFF:
            led_3 = WS2812::NONE;
            break;
        case MidiRole::BIAS:
            led_3 = WS2812::GREEN;
            break;
        case MidiRole::EVENTS:
            led_3 = WS2812::ORANGE;
            break;
        case MidiRole::HYBRID:
        case MidiRole::COUNT:
            led_3 = WS2812::CYAN;
            break;
        }
    }

    if (scene_feedback_ticks_ > 0)
    {
        led_1 = mode_colors_[current_mode_];
        led_2 = scene_feedback_store_ ? WS2812::WHITE : (scene_morph_active_ ? WS2812::CYAN : WS2812::BLUE);
        led_3 = kSceneColors[active_scene_ % kSceneColors.size()];
        scene_feedback_ticks_--;
    }

    Kastle2::hw.SetLed(Hardware::Led::LED_1, led_1);
    Kastle2::hw.SetLed(Hardware::Led::LED_2, led_2);
    Kastle2::hw.SetLed(Hardware::Led::LED_3, led_3);
}

void AppStackBrain::MidiCallback(midi::Message *msg)
{
    for (auto &pot : pots_)
    {
        pot->MidiCallback(msg);
    }

    if (msg->IsControlChange())
    {
        if (msg->GetData1() == kMidiCcSceneSelect)
        {
            active_scene_ = constrain<uint8_t>(map(static_cast<int32_t>(msg->GetData2()), 0, 127, 0, kSceneCount - 1), 0, kSceneCount - 1);
            Kastle2::memory.QueueUpdate8(kMemActiveScene, active_scene_);
            scene_feedback_ticks_ = kSceneFeedbackTicks;
            scene_feedback_store_ = false;
            return;
        }

        if (msg->GetData1() == kMidiCcSceneRecall && msg->GetData2() >= 64)
        {
            RecallScene(active_scene_);
            return;
        }

        if (msg->GetData1() == kMidiCcSceneStore && msg->GetData2() >= 64)
        {
            StoreScene(active_scene_);
            return;
        }
    }

    bool midi_bias_enabled = midi_role_ == MidiRole::BIAS || midi_role_ == MidiRole::HYBRID;
    bool midi_events_enabled = midi_role_ == MidiRole::EVENTS || midi_role_ == MidiRole::HYBRID;

    if (msg->IsStart() || msg->IsContinue())
    {
        if (midi_events_enabled)
        {
            midi_clock_running_ = true;
            midi_clock_counter_ = 0;
        }
        return;
    }

    if (msg->IsStop())
    {
        midi_clock_running_ = false;
        midi_clock_counter_ = 0;
        midi_clock_event_pending_ = false;
        return;
    }

    if (msg->IsClock() && midi_clock_running_)
    {
        midi_clock_counter_++;
        if (midi_clock_counter_ >= std::max<uint32_t>(1, midi_clock_divider_))
        {
            midi_clock_counter_ = 0;
            midi_clock_event_pending_ = true;
        }
        return;
    }

    if (msg->IsNoteOn())
    {
        if (midi_bias_enabled)
        {
            midi_note_cv_ = constrain(map(static_cast<int32_t>(msg->GetData1()), 36, 96, -POT_HALF, POT_HALF), -POT_HALF, POT_HALF);
            midi_velocity_cv_ = map(static_cast<int32_t>(msg->GetData2()), 0, 127, 0, POT_HALF);
        }
        if (midi_events_enabled)
        {
            midi_follow_event_pending_ = true;
        }
        return;
    }

    if (msg->IsNoteOff())
    {
        if (midi_bias_enabled)
        {
            midi_velocity_cv_ = 0;
        }
        return;
    }

    if (msg->IsPitchBend() && midi_bias_enabled)
    {
        midi_note_cv_ = constrain(static_cast<int32_t>(msg->GetPitchBendAsFloat() * static_cast<float>(POT_HALF)), -POT_HALF, POT_HALF);
    }
}

void AppStackBrain::MemoryInitialization()
{
    Kastle2::memory.Write8(kMemMode, std::to_underlying(Mode::STEP));
    Kastle2::memory.Write8(kMemRateMod, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemShapeMod, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemSlew, pot_to_mem(POT_THIRD));
    Kastle2::memory.Write8(kMemResponse, pot_to_mem(POT_TWO_THIRDS));
    Kastle2::memory.Write8(kMemRate, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemRange, pot_to_mem(POT_TWO_THIRDS));
    Kastle2::memory.Write8(kMemDensity, pot_to_mem(POT_TWO_THIRDS));
    Kastle2::memory.Write8(kMemClockDefault, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemLinkDefault, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemRouteDefault, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemMidiRole, pot_to_mem(POT_MAX));
    Kastle2::memory.Write8(kMemBiasRoute, pot_to_mem(POT_MIN));
    Kastle2::memory.Write8(kMemPersonality, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemActiveScene, 0);

    const auto get_scene_value = [](const uint8_t slot, const Pot pot_id) -> int32_t {
        switch (pot_id)
        {
        case Pot::RATE_MOD:
        case Pot::SHAPE_MOD:
        case Pot::EVENT_BIAS:
        case Pot::PITCH_1_DEPTH:
        case Pot::PITCH_2_DEPTH:
        case Pot::MIDI_NOTE_DEPTH:
        case Pot::MIDI_VELOCITY_DEPTH:
            return slot == 3 ? POT_TWO_THIRDS : POT_HALF;
        case Pot::SLEW:
        case Pot::LINK:
        case Pot::MIDI_CLOCK_DIV:
            return POT_THIRD;
        case Pot::RESPONSE:
            return slot == 2 ? POT_MAX : (slot == 3 ? (POT_MAX * 3 / 4) : POT_TWO_THIRDS);
        case Pot::RATE:
            return slot == 1 ? (POT_MAX * 3 / 4) : (slot == 3 ? POT_TWO_THIRDS : POT_HALF);
        case Pot::RANGE:
            return POT_TWO_THIRDS;
        case Pot::DENSITY:
            return slot == 1 ? POT_HALF : (slot == 2 ? POT_MAX : POT_TWO_THIRDS);
        case Pot::CLOCK_DEFAULT:
            return slot == 3 ? POT_MAX : POT_HALF;
        case Pot::LINK_DEFAULT:
            return slot == 3 ? POT_MAX : POT_HALF;
        case Pot::ROUTE_DEFAULT:
            return slot == 2 || slot == 3 ? POT_MAX : POT_HALF;
        case Pot::MIDI_ROLE:
            return POT_MAX;
        case Pot::BIAS_ROUTE:
            return slot == 3 ? POT_MAX : POT_MIN;
        case Pot::PERSONALITY:
            return slot == 2 ? POT_TWO_THIRDS : (slot == 3 ? POT_MAX : POT_HALF);
        case Pot::MODE_SELECT:
        case Pot::COUNT:
            break;
        }
        return POT_HALF;
    };

    const auto get_scene_mode = [](const uint8_t slot) -> Mode {
        switch (slot)
        {
        case 0:
            return Mode::STEP;
        case 1:
            return Mode::FOLLOW;
        case 2:
            return Mode::CHAOS;
        case 3:
        default:
            return Mode::STEP;
        }
    };

    for (uint8_t slot = 0; slot < kSceneCount; slot++)
    {
        Kastle2::memory.Write8(GetSceneAddress(slot, 0), std::to_underlying(get_scene_mode(slot)));
        size_t offset = 1;
        for (auto pot_id : EnumRange<Pot>())
        {
            if (pot_id == Pot::MODE_SELECT)
            {
                continue;
            }

            Kastle2::memory.Write8(GetSceneAddress(slot, offset), pot_to_mem(get_scene_value(slot, pot_id)));
            offset++;
        }
    }
}
