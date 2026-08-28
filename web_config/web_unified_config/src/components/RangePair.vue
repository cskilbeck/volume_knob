<!--
  Two values on one track, with a slider handle and a fine-adjust spinner each.

  Used for both of the adapter's value pairs: the expression pedal's output
  range (heel..toe) and the footswitch's two output values (off/on).

  Whether the handles may cross is the caller's decision, via `ordered`:

    ordered = false (default)  either handle may pass the other. Right for the
                               footswitch, where "off" above "on" simply means
                               the switch sends the higher value when open.

    ordered = true             low stays at least `minSpan` below high. Right
                               for the pedal's output range, where a crossed
                               pair reads as a mistake rather than as intent.

  Under `ordered` the handles CLAMP rather than push: dragging one into the
  other stops it, instead of shoving a value the user was not editing. Clamping
  cannot repair a pair that arrives already crossed, so a caller that enables
  this should normalise its own values once when they are loaded.

  The fill is drawn between whichever handle is lower and whichever is higher
  either way, so it stays correct when crossing is permitted.
-->

<script setup>

import { computed } from 'vue';

const props = defineProps({
    low:       { type: Number, required: true },
    high:      { type: Number, required: true },
    max:       { type: Number, default: 127 },
    lowLabel:  { type: String, default: 'Low' },
    highLabel: { type: String, default: 'High' },
    disabled:  { type: Boolean, default: false },
    ordered:   { type: Boolean, default: false },
    minSpan:   { type: Number, default: 1 },
});

const emit = defineEmits(['update:low', 'update:high']);

function clamp(v) {
    if (!Number.isFinite(v)) {
        return 0;
    }
    return Math.max(0, Math.min(props.max, Math.round(v)));
}

function set_low(v) {
    v = clamp(v);
    if (props.ordered) {
        // Never below 0, even if `high` is itself too low to leave room —
        // that case is the caller's to normalise, not ours to make worse.
        v = Math.max(0, Math.min(v, props.high - props.minSpan));
    }
    emit('update:low', v);
}

function set_high(v) {
    v = clamp(v);
    if (props.ordered) {
        v = Math.min(props.max, Math.max(v, props.low + props.minSpan));
    }
    emit('update:high', v);
}

const fill = computed(() => {
    const lo = Math.min(props.low, props.high) / props.max * 100;
    const hi = Math.max(props.low, props.high) / props.max * 100;
    return { left: `${lo}%`, width: `${hi - lo}%` };
});

</script>

<template>
    <div class="rp" :class="{ 'rp-disabled': disabled }">

        <div class="rp-track">
            <div class="rp-rail"></div>
            <div class="rp-fill" :style="fill"></div>

            <input type="range" min="0" :max="max" :disabled="disabled"
                   :value="low" :aria-label="lowLabel"
                   @input="set_low(Number($event.target.value))">

            <input type="range" min="0" :max="max" :disabled="disabled"
                   :value="high" :aria-label="highLabel"
                   @input="set_high(Number($event.target.value))">
        </div>

        <div class="row g-2 mt-1">
            <div class="col">
                <div class="input-group input-group-sm flex-nowrap">
                    <span class="input-group-text user-select-none rp-tag">{{ lowLabel }}</span>
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set_low(low - 1)">&minus;</button>
                    <input type="number" class="form-control text-center" min="0" :max="max"
                           :disabled="disabled" :value="low"
                           @change="set_low(Number($event.target.value))">
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set_low(low + 1)">+</button>
                </div>
            </div>
            <div class="col">
                <div class="input-group input-group-sm flex-nowrap">
                    <span class="input-group-text user-select-none rp-tag">{{ highLabel }}</span>
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set_high(high - 1)">&minus;</button>
                    <input type="number" class="form-control text-center" min="0" :max="max"
                           :disabled="disabled" :value="high"
                           @change="set_high(Number($event.target.value))">
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set_high(high + 1)">+</button>
                </div>
            </div>
        </div>
    </div>
</template>

<style scoped>
.rp-track {
    position: relative;
    height: 1.5rem;
}

.rp-rail,
.rp-fill {
    position: absolute;
    top: 50%;
    height: 4px;
    margin-top: -2px;
    border-radius: 2px;
}

.rp-rail {
    left: 0;
    right: 0;
    background-color: var(--bs-secondary-bg);
}

.rp-fill {
    background-color: var(--bs-primary);
}

/*
 * Both sliders occupy the same space, stacked. The INPUT is transparent to the
 * mouse and only its thumb accepts events, otherwise whichever input is on top
 * would swallow every click and the lower handle could never be grabbed.
 * Keyboard focus is unaffected by pointer-events, so both remain reachable by
 * tabbing.
 */
.rp-track input[type=range] {
    position: absolute;
    left: 0;
    top: 0;
    width: 100%;
    height: 1.5rem;
    margin: 0;
    background: none;
    pointer-events: none;
    -webkit-appearance: none;
    appearance: none;
}

.rp-track input[type=range]:focus {
    outline: none;
}

.rp-track input[type=range]::-webkit-slider-thumb {
    pointer-events: auto;
    -webkit-appearance: none;
    appearance: none;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    border: 1px solid var(--bs-border-color);
    background-color: var(--bs-primary);
    cursor: pointer;
}

.rp-track input[type=range]::-moz-range-thumb {
    pointer-events: auto;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    border: 1px solid var(--bs-border-color);
    background-color: var(--bs-primary);
    cursor: pointer;
}

.rp-track input[type=range]:focus-visible::-webkit-slider-thumb {
    box-shadow: 0 0 0 3px rgba(var(--bs-primary-rgb), 0.4);
}

.rp-track input[type=range]:focus-visible::-moz-range-thumb {
    box-shadow: 0 0 0 3px rgba(var(--bs-primary-rgb), 0.4);
}

.rp-tag {
    min-width: 3.2rem;
}

/* Room for four digits: these hold raw ADC counts in the footswitch case. */
.rp input[type=number] {
    min-width: 3.5rem;
}

.rp-disabled {
    opacity: 0.5;
}
</style>
