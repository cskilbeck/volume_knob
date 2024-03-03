<script setup>

import { ref, watchEffect } from 'vue';

const props = defineProps({
    modelValue: {
        type: Boolean,
        required: true
    },
    pixelsWide: {
        type: Number,
        required: false,
        default: 128
    },
    pixelsHigh: {
        type: Number,
        required: false,
        default: 32
    },
    dotSize: {
        type: Number,
        required: false,
        default: 0.75
    },
    marginPixels: {
        type: Number,
        required: false,
        default: 4
    },
    rounded: {
        type: Number,
        required: false,
        default: 0
    }
});

let isChecked = ref(false);

const emits = defineEmits(["update:modelValue"]);

watchEffect(function () {
    isChecked.value = props.modelValue;
});

function dotSize() {
    return props.pixelsHigh * props.dotSize;
}

function dotBorder() {
    return props.pixelsHigh * 0.5 - dotSize() * 0.5;
}

</script>

<template>
    <label class="switch">
        <input v-model="isChecked" type="checkbox" @change="emits('update:modelValue', isChecked)">
        <span class="slider">
            <span class='checked-text'>
                <slot name='checked-text'></slot>
            </span>
            <span class='unchecked-text'>
                <slot name='unchecked-text'></slot>
            </span>
        </span>
    </label>
</template>

<style scoped>
.switch {
    position: relative;
    display: inline-block;
    width: v-bind('props.pixelsWide + "px"');
    height: v-bind('props.pixelsHigh + "px"');
    font-size: smaller;
}

.switch input {
    visibility: hidden;
}

.slider {
    border: 0px solid;
    border-color: var(--bs-border-color);
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: var(--bs-secondary-bg);
    transition: 0.2s;
    overflow: hidden;
    border-radius: v-bind('(props.pixelsHigh * props.rounded * 0.5) + "px"');
}

.slider:before {
    position: absolute;
    content: "";
    margin: 0px;
    padding: 0px;
    height: v-bind('dotSize() + "px"');
    width: v-bind('dotSize() + "px"');
    left: 0%;
    top: 50%;
    border-radius: v-bind('(dotSize() * props.rounded * 0.5) + "px"');
    background-color: var(--bs-body-bg);
    transform: v-bind('"translate(" + dotBorder() + "px, -50%)"');
    transition: 0.2s;
}

input:checked+.slider {
    background-color: var(--bs-secondary-bg);
}

span.unchecked-text {
    user-select: none;
}

span.checked-text {
    user-select: none;
}

input+.slider span {
    position: absolute;
    top: 50%;
}

input:checked+.slider:before {
    transform: v-bind('"translate(" + (props.pixelsWide - dotSize() - dotBorder()) + "px, -50%)"');
}

/* Checked text on the right */
input:checked+.slider span.checked-text {
    opacity: 100%;
    right: 100%;
    transition: 0.2s;
    transform: v-bind('"translate(" + (props.pixelsWide - (dotBorder() + dotSize() + props.marginPixels)) + "px, -50%)"');
}

/* Checked text hidden on the left */
input+.slider span.checked-text {
    opacity: 0%;
    right: 100%;
    transition: 0.2s;
    transform: v-bind('"translate(" + (dotBorder() - props.marginPixels) + "px, -50%"');
}

/* Unchecked text on the left */
input+.slider span.unchecked-text {
    opacity: 100%;
    transition: 0.2s;
    transform: v-bind('"translate(" + (dotBorder() + dotSize() + props.marginPixels) + "px, -50%)"');
}

/* Unchecked text hidden on the right */
input:checked+.slider span.unchecked-text {
    opacity: 0%;
    transition: 0.2s;
    transform: v-bind('"translate(" + (props.pixelsWide - dotBorder() + props.marginPixels) + "px, -50%)"');
}
</style>