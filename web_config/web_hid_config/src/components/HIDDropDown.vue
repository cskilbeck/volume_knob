<!-- HIDDropDown - show a list of hid keys in a dropdown with an input -->

<script setup>

import { ref, watch, watchEffect } from 'vue';
import keys from '../hid_keys.js';

const props = defineProps({
    modelValue: {
        type: Number,
        required: true,
        default: 0
    }
});

const emit = defineEmits(["update:modelValue"]);

const current = ref(0);

watchEffect(function () {
    current.value = props.modelValue;
});

watch(current, (n) => {
    current.value = n;
    emit('update:modelValue', current.value);
});

</script>

<template>
    <div class="input-group limit-height">

        <input type="number" class="form-control" v-model.number="current">

        <button class="btn tertiary-bg small-text border rounded-end form-select ps-2 pe-0 text-start" type="button"
            data-bs-toggle="dropdown">
            <slot></slot>
            <span class="d-inline-block"></span>
        </button>

        <ul class="dropdown-menu border-secondary-subtle rounded-0" style="width:400px">

            <li v-for="(name, val) in keys.hid_keys" class="w-100">
                <a class="dropdown-item small-text" href="#" @click="current = val | 0">
                    <div class="d-flex flex-row">
                        <div class="w-75 me-0">
                            {{ name }}
                        </div>
                        <div class="me-2">
                            {{ val.toString(16).toUpperCase().padStart(4, '0') }}
                        </div>
                        <span class="w-25 rounded mx-0 px-0 bg-secondary-subtle text-center">
                            Keyboard
                        </span>
                    </div>
                </a>
            </li>
            <li v-for="(name, val) in keys.consumer_control_keys" class="w-100">
                <a class="dropdown-item small-text" href="#" @click="current = val | 0x8000">
                    <div class="d-flex flex-row">
                        <div class="w-75 me-0">
                            {{ name }}
                        </div>
                        <div class="me-2">
                            {{ val.toString(16).toUpperCase().padStart(4, '0') }}
                        </div>
                        <span class="w-25 rounded mx-0 px-0 bg-secondary-subtle text-center">
                            CC Key
                        </span>
                    </div>
                </a>
            </li>

        </ul>
    </div>
    <div class="row mb-2 me-0 small-text text-secondary">
        <div class="col pe-0">
            {{ ((current & 0x8000) == 0) ? keys.hid_keys[current] : keys.consumer_control_keys[current] }}
        </div>
    </div>
</template>

<style>
.hide {
    visibility: hidden;
}

.small-text {
    font-size: small;
    color: var(--bs-body-color);
}

.tertiary-bg {
    background-color: var(--bs-tertiary-bg);
}

.tertiary-bg:hover {
    background-color: var(--bs-secondary-bg)
}

.limit-height ul {
    max-height: 250px;
    overflow-y: auto;
}
</style>
