<script setup>

//////////////////////////////////////////////////////////////////////

import { onMounted, ref, nextTick, watchEffect, computed } from 'vue';

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
        description: "Font size for the checked/unchecked text"
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
        description: "Horizontal spacing between pill and text in pixels"
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

const isChecked = ref(false);

const emit = defineEmits(["update:modelValue"]);

watchEffect(function () {
    isChecked.value = props.modelValue;
});

//////////////////////////////////////////////////////////////////////

const ready = ref(false);   // ready for avoiding initial animation problem
const theLabel = ref(null);
const labelWidth = ref(0);
const labelHeight = ref(0);

//////////////////////////////////////////////////////////////////////

function updateDimensions() {
    let wpx = "0px";
    let hpx = "0px";
    if (theLabel.value) {
        wpx = getComputedStyle(theLabel.value).width;
        hpx = getComputedStyle(theLabel.value).height;
    }
    // wpx,hpx are in the format "#px" - is it guaranteed?
    labelWidth.value = wpx.substring(0, wpx.length - 2) | 0;
    labelHeight.value = hpx.substring(0, hpx.length - 2) | 0;
}

//////////////////////////////////////////////////////////////////////

const trans = computed(() => { return ready.value ? props.transitionTime : 0 });

const dotSize = computed(() => { return (labelHeight.value - props.borderPixels * 2) * props.pillSize; });

const dotBorder = computed(() => { return ((labelHeight.value - props.pillSize) - dotSize.value) * 0.5; });

//////////////////////////////////////////////////////////////////////

onMounted(() => {

    updateDimensions();

    new ResizeObserver((entries) => {
        updateDimensions();
    }).observe(theLabel.value);

    // allow a moment to get the initial translation done
    // couldn't make this work with nextTick()...
    setTimeout(() => { ready.value = true; }, 0);
});

//////////////////////////////////////////////////////////////////////

</script>

<template>
    <label class="toggle" ref="theLabel">
        <input v-model="isChecked" type="checkbox" @change="emit('update:modelValue', isChecked)">
        <span class="pill">
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
.toggle {
    position: relative;
    font-size: v-bind('props.fontSize');
    display: block;
    margin: 0px;
    padding: 0px;
    width: 100%;
    height: 100%;
}

.toggle input {
    visibility: hidden;
}

.pill {
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
    border-radius: v-bind('`${(labelHeight * props.rounded * 0.5)}px`');
}

.pill:before {
    position: absolute;
    content: "";
    margin: 0px;
    padding: 0px;
    height: v-bind('`${dotSize}px`');
    width: v-bind('`${dotSize}px`');
    left: 0%;
    top: 50%;
    border-radius: v-bind('`${dotSize * props.rounded * 0.5}px`');
    background-color: v-bind('`${props.uncheckedPillColor}`');
    transform: v-bind('`translate(${dotBorder - props.borderPixels + 0.5}px, -50%)`');
    transition: v-bind('`transform ${trans}s, background-color ${trans}s`');
}

input:checked+.pill {
    background-color: v-bind('`${props.checkedBackgroundColor}`');
}

/* spans for containing checked and unchecked text */
input+.pill span {
    position: absolute;
    top: 50%;
    user-select: none;
    transition: v-bind('`transform ${trans}s, opacity ${trans}s`');
}

/* checked pill */
input:checked+.pill:before {
    background-color: v-bind('`${props.checkedPillColor}`');
    transform: v-bind('`translate(${labelWidth - (dotBorder + dotSize + props.borderPixels)}px, -50%)`');
}

/* unchecked: checked text hidden on the left */
input+.pill span.checked-text-span {
    opacity: 0%;
    right: 100%;
    transform: v-bind('`translate(${dotBorder - props.marginPixels - props.borderPixels}px, -50%`');
}

/* unchecked: unchecked text shown on the right */
input+.pill span.unchecked-text-span {
    opacity: 100%;
    transform: v-bind('`translate(${dotBorder + dotSize + props.marginPixels - props.borderPixels}px, -50%)`');
}

/* checked: checked text shown on the left */
input:checked+.pill span.checked-text-span {
    opacity: 100%;
    right: 100%;
    transform: v-bind('`translate(${labelWidth - (dotBorder + dotSize + props.marginPixels + props.borderPixels)}px, -50%)`');
}

/* checked: unchecked text hidden on the right */
input:checked+.pill span.unchecked-text-span {
    opacity: 0%;
    transform: v-bind('`translate(${labelWidth - dotBorder + props.marginPixels - props.borderPixels}px, -50%)`');
}
</style>