<!--
  One value: a slider, plus a spinner with explicit -/+ buttons for fine
  adjustment (a slider alone cannot reliably hit an exact number, and the
  native arrow keys need focus first).

  Deliberately NOT built from RangePair. That component needs two overlapping
  <input type=range> with pointer-events juggled so both thumbs stay grabbable;
  a single value needs none of that and a plain Bootstrap .form-range does the
  job.
-->

<script setup>

const props = defineProps({
    modelValue: { type: Number, required: true },
    min:        { type: Number, default: 0 },
    max:        { type: Number, default: 100 },
    label:      { type: String, default: '' },
    suffix:     { type: String, default: '' },
    disabled:   { type: Boolean, default: false },
});

const emit = defineEmits(['update:modelValue']);

function set(v) {
    if (!Number.isFinite(v)) {
        v = props.min;
    }
    emit('update:modelValue',
         Math.max(props.min, Math.min(props.max, Math.round(v))));
}

</script>

<template>
    <div>
        <div class="row g-2">
            <div class="col">
                <!-- flex-nowrap so this can never fold into a second line and
                     clip the number: shrinking the field is bad, hiding it is
                     worse. -->
                <div class="input-group input-group-sm flex-nowrap">
                    <span class="input-group-text user-select-none vs-tag" v-if="label">{{ label }}</span>
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set(modelValue - 1)">&minus;</button>
                    <input type="number" class="form-control text-center vs-num" :min="min" :max="max"
                           :disabled="disabled" :value="modelValue"
                           @change="set(Number($event.target.value))">
                    <button class="btn tertiary-bg border border-secondary-subtle" type="button"
                            :disabled="disabled" @click="set(modelValue + 1)">+</button>
                    <span class="input-group-text user-select-none" v-if="suffix">{{ suffix }}</span>
                </div>
            </div>
        </div>
        <input type="range" class="form-range mt-1" :min="min" :max="max"
               :disabled="disabled" :value="modelValue"
               :aria-label="label"
               @input="set(Number($event.target.value))">
    </div>
</template>

<style scoped>
.vs-tag {
    min-width: 3.2rem;
}

/* Wide enough for four digits, so the raw-count case (0..4095) is readable and
 * flex-nowrap has something to shrink towards rather than past. */
.vs-num {
    min-width: 3.5rem;
}
</style>
