<script setup>

import { onMounted, ref, watchEffect, computed } from 'vue';

const props = defineProps({
    modelValue: {
        type: Boolean,
        required: true,
        description: "The model value"
    },
    pillSize: {
        type: Number,
        required: false,
        default: 0.75,
        description: "The pill size, 0..1 where 1 = same size as vertical extent"
    },
    marginPixels: {
        type: Number,
        required: false,
        default: 6,
        description: "Horizontal spacing between pill and text"
    },
    borderPixels: {
        type: Number,
        required: false,
        default: 1,
        description: "Thickness of the border in pixels"
    },
    rounded: {
        type: Number,
        required: false,
        default: 1,
        description: "How rounded, 0..1 where 0 = square, 1 = rounded"
    },
    transitionTime: {
        type: Number,
        required: false,
        default: 0.3,
        description: "How long transition between checked/unchecked takes"
    },
    checkedPillColor: {
        type: String,
        required: false,
        default: "White",
        description: "Color of the pill when checked"
    },
    uncheckedPillColor: {
        type: String,
        required: false,
        default: "White",
        description: "Color of the pill when unchecked"
    },
    checkedTextColor: {
        type: String,
        required: false,
        default: "White",
        description: "Color of the text when checked"
    },
    uncheckedTextColor: {
        type: String,
        required: false,
        default: "White",
        description: "Color of the text when unchecked"
    },
    checkedBackgroundColor: {
        type: String,
        required: false,
        default: "#45F",
        description: "Background color when checked"
    },
    uncheckedBackgroundColor: {
        type: String,
        required: false,
        default: "#555",
        description: "Background color when unchecked"
    },
    borderColor: {
        type: String,
        required: false,
        default: "#AAA",
        description: "Border color"
    }
});

const isChecked = ref(false);

const emits = defineEmits(["update:modelValue"]);

watchEffect(function () {
    isChecked.value = props.modelValue;
});

let ready = ref(false);

const theLabel = ref(null);
const labelWidth = ref(0);
const labelHeight = ref(0);
const trans = computed(() => { return ready.value ? props.transitionTime : 0 });
const pixelsWide = computed(() => { return labelWidth.value; });
const pixelsHigh = computed(() => { return labelHeight.value; });
const dotSize = computed(() => { return getDotSize(); });
const dotBorder = computed(() => { return getDotBorder(); });

function updateDimensions() {
    let wpx = "0px";
    let hpx = "0px";
    if (theLabel.value) {
        wpx = getComputedStyle(theLabel.value).width;
        hpx = getComputedStyle(theLabel.value).height;

        // this is nasty...
        setTimeout(() => { ready.value = true; }, 0);
    }
    // wpx,hpx are in the format "#px" - is it guaranteed?
    labelWidth.value = wpx.substring(0, wpx.length - 2) | 0;
    labelHeight.value = hpx.substring(0, hpx.length - 2) | 0;
}

function getDotSize() {
    return (labelHeight.value - props.borderPixels * 2) * props.pillSize;
}

function getDotBorder() {
    return ((labelHeight.value - props.pillSize) - getDotSize()) * 0.5;
}

const v_offset = "-50%";

const resizeObserver = new ResizeObserver((entries) => {

    updateDimensions();
});

onMounted(() => {
    resizeObserver.observe(theLabel.value);

});

</script>

<template>
    <label class="switch" ref="theLabel">
        <input v-model="isChecked" type="checkbox" @change="emits('update:modelValue', isChecked)">
        <span class="slider">
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
.switch {
    position: relative;
    font-size: smaller;
    display: block;
    margin: 0px;
    padding: 0px;
    width: 100%;
    height: 100%;
}

.switch input {
    visibility: hidden;
}

.slider {
    border: v-bind('props.borderColor') v-bind('(props.borderPixels) + "px"') solid;
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: v-bind('`${props.uncheckedBackgroundColor}`');
    transition: transform v-bind('`${trans}s`'),
    background-color v-bind('`${trans}s`');
    overflow: hidden;
    border-radius: v-bind('(pixelsHigh * props.rounded * 0.5) + "px"');
}

.slider:before {
    position: absolute;
    content: "";
    margin: 0px;
    padding: 0px;
    height: v-bind('dotSize + "px"');
    width: v-bind('dotSize + "px"');
    left: 0%;
    top: 50%;
    border-radius: v-bind('(dotSize * props.rounded * 0.5) + "px"');
    background-color: v-bind('`${props.uncheckedPillColor}`');
    transform: v-bind('`translate(${dotBorder - props.borderPixels + 0.5}px, ${v_offset})`');
    transition: transform v-bind('`${trans}s`'),
    background-color v-bind('`${trans}s`');
}

input:checked+.slider {
    background-color: v-bind('`${props.checkedBackgroundColor}`');
}

input+.slider span {
    position: absolute;
    top: 50%;
    user-select: none;
}

input:checked+.slider:before {
    background-color: v-bind('`${props.checkedPillColor}`');
    transform: v-bind('`translate(${pixelsWide - (dotBorder + dotSize + props.borderPixels)}px, ${v_offset})`');
}

/* unchecked: checked text hidden on the left */
input+.slider span.checked-text-span {
    opacity: 0%;
    right: 100%;
    color: v-bind('props.checkedTextColor');
    transition: transform v-bind('`${trans}s,`') opacity v-bind('`${trans}s`');
    transform: v-bind('`translate(${dotBorder - props.marginPixels - props.borderPixels}px, ${v_offset}`');
}

/* unchecked: unchecked text shown on the right */
input+.slider span.unchecked-text-span {
    opacity: 100%;
    color: v-bind('props.uncheckedTextColor');
    transition: transform v-bind('`${trans}s,`') opacity v-bind('`${trans}s`');
    transform: v-bind('`translate(${dotBorder + dotSize + props.marginPixels - props.borderPixels}px, ${v_offset})`');
}

/* checked: checked text shown on the left */
input:checked+.slider span.checked-text-span {
    opacity: 100%;
    right: 100%;
    transition: transform v-bind('`${trans}s,`') opacity v-bind('`${trans}s`');
    transform: v-bind('`translate(${pixelsWide - (dotBorder + dotSize + props.marginPixels + props.borderPixels)}px, ${v_offset})`');
}

/* checked: unchecked text hidden on the right */
input:checked+.slider span.unchecked-text-span {
    opacity: 0%;
    transition: transform v-bind('`${trans}s,`') opacity v-bind('`${trans}s`');
    transform: v-bind('`translate(${pixelsWide - dotBorder + props.marginPixels - props.borderPixels}px, ${v_offset})`');
}
</style>