<script setup>

//////////////////////////////////////////////////////////////////////

import { onMounted, ref, watchEffect, computed } from 'vue';

//////////////////////////////////////////////////////////////////////

const props = defineProps({
    modelValue: {
        type: Boolean,
        required: true,
        description: "The model value"
    },
    fontSize: {
        type: String,
        required: false,
        default: "smaller",
        description: "Font size for the text"
    },
    pillSizePercent: {
        type: Number,
        required: false,
        default: 70,
        description: "Pill size, 0..100%"
    },
    marginPixels: {
        type: Number,
        required: false,
        default: 6,
        description: "Horizontal spacing between pill and text in pixels"
    },
    borderPixels: {
        type: Number,
        required: false,
        default: 1,
        description: "Thickness of the border in pixels"
    },
    roundedPercent: {
        type: Number,
        required: false,
        default: 100,
        description: "How rounded, 0..100% where 0 = square, 100 = rounded"
    },
    transitionTime: {
        type: Number,
        required: false,
        default: 0.25,
        description: "TTransition time between checked/unchecked"
    },
    checkedPillColor: {
        type: String,
        required: false,
        default: "White",
        description: "Pill color when checked"
    },
    uncheckedPillColor: {
        type: String,
        required: false,
        default: "White",
        description: "Pill color when unchecked"
    },
    checkedBackgroundColor: {
        type: String,
        required: false,
        default: "#359",
        description: "Background color when checked"
    },
    uncheckedBackgroundColor: {
        type: String,
        required: false,
        default: "#3C4244",
        description: "Background color when unchecked"
    },
    borderColor: {
        type: String,
        required: false,
        default: "#888",
        description: "Border color"
    }
});

//////////////////////////////////////////////////////////////////////
// track modelValue

const isChecked = ref(false);

const emit = defineEmits(["update:modelValue"]);

watchEffect(function () { isChecked.value = props.modelValue; });

//////////////////////////////////////////////////////////////////////
// transition time suppressed to 0 until onMounted

const ready = ref(false);

onMounted(() => { ready.value = true; });

const trans = computed(() => { return ready.value ? props.transitionTime : 0 });

//////////////////////////////////////////////////////////////////////
// the top level dom object, needs this for dimensions

const theLabel = ref(null);

// size of the pill in pixels
const pill_size_px = computed(() => { return (theLabel.value.offsetHeight - props.borderPixels * 2) * props.pillSizePercent / 100; });

// space between pill and border in pixels
const pill_border_px = computed(() => { return ((theLabel.value.offsetHeight - props.pillSizePercent / 100) - pill_size_px.value) * 0.5; });

//////////////////////////////////////////////////////////////////////

</script>

<template>
    <label class="toggle" ref="theLabel">
        <input v-model="isChecked" type="checkbox" @change="emit('update:modelValue', isChecked)">
        <span class="pill-container">
            <span class='checked-text-span'>
                <slot name="checked"></slot>
            </span>
            <span class='unchecked-text-span'>
                <slot name="unchecked"></slot>
            </span>
        </span>
    </label>
</template>

<style scoped>
/* main toggle element */
.toggle {
    position: relative;
    font-size: v-bind('props.fontSize');
    display: block;
    margin: 0px;
    padding: 0px;
    width: 100%;
    height: 100%;
}

/* hide the actual checkbox */
.toggle input {
    visibility: hidden;
}

/* the actual top level visible bit */
.pill-container {
    border: v-bind('`${props.borderColor} ${props.borderPixels}px solid`');
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: v-bind('`${props.uncheckedBackgroundColor}`');
    transition: v-bind('`transform ${trans}s, background-color ${trans}s`');
    overflow: hidden;
    border-radius: v-bind('`${(theLabel.offsetHeight * 0.005 * props.roundedPercent)}px`');
}

/* main pill setup + unchecked pill color */
.pill-container:before {
    position: absolute;
    content: "";
    margin: 0px;
    padding: v-bind('`${pill_size_px * 0.5}px`');
    width: 0px;
    height: 0px;
    left: 0%;
    top: 50%;
    border-radius: v-bind('`${props.roundedPercent * 0.5}%`');
    transform: v-bind('`translate(${pill_border_px - props.borderPixels}px, -50%)`');
    transition: v-bind('`transform ${trans}s, background-color ${trans}s`');
    background-color: v-bind('`${props.uncheckedPillColor}`');
}

/* checked bg color */
input:checked+.pill-container {
    background-color: v-bind('`${props.checkedBackgroundColor}`');
}

/* spans for containing checked and unchecked text */
input+.pill-container span {
    position: absolute;
    top: 50%;
    user-select: none;
    transition: v-bind('`transform ${trans}s, opacity ${trans}s`');
}

/* checked pill, set color and position */
input:checked+.pill-container:before {
    background-color: v-bind('`${props.checkedPillColor}`');
    transform: v-bind('`translate(${theLabel.offsetWidth - (pill_border_px + pill_size_px + props.borderPixels)}px, -50%)`');
}

/* unchecked: checked text hidden on the left */
input+.pill-container span.checked-text-span {
    opacity: 0%;
    right: 100%;
    transform: v-bind('`translate(${-props.marginPixels - props.borderPixels}px, -50%`');
}

/* unchecked: unchecked text shown on the right */
input+.pill-container span.unchecked-text-span {
    opacity: 100%;
    transform: v-bind('`translate(${(pill_border_px * 2) + pill_size_px + props.marginPixels - props.borderPixels}px, -50%)`');
}

/* checked: checked text shown on the left */
input:checked+.pill-container span.checked-text-span {
    opacity: 100%;
    right: 100%;
    transform: v-bind('`translate(${theLabel.offsetWidth - ((pill_border_px * 2) + pill_size_px + props.marginPixels + props.borderPixels)}px, -50%)`');
}

/* checked: unchecked text hidden on the right */
input:checked+.pill-container span.unchecked-text-span {
    opacity: 0%;
    transform: v-bind('`translate(${theLabel.offsetWidth + props.marginPixels - props.borderPixels}px, -50%)`');
}
</style>