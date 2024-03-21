<!-- HIDDropDown - show a list of hid keys in a dropdown with an input -->

<script setup>

//////////////////////////////////////////////////////////////////////

import { computed, ref, watch, watchEffect } from 'vue';
import keys from '../hid_keys.js';

//////////////////////////////////////////////////////////////////////

const props = defineProps({
    keycode: {
        type: Number,
        required: true,
        default: 0
    },
    mod: {
        type: Number,
        required: true,
        default: 0
    }
});

//////////////////////////////////////////////////////////////////////

const emit = defineEmits(["update:keycode", "update:mod"]);

//////////////////////////////////////////////////////////////////////

const current_keycode = ref(0);
const current_mod = ref(0);

//////////////////////////////////////////////////////////////////////

watchEffect(function () {
    current_keycode.value = props.keycode;
    current_mod.value = props.mod;
});

//////////////////////////////////////////////////////////////////////

watch(current_keycode, (n) => {
    current_keycode.value = n;
    emit('update:keycode', current_keycode.value);
});

//////////////////////////////////////////////////////////////////////

watch(current_mod, (n) => {
    current_mod.value = n;
    emit('update:mod', current_mod.value);
});

//////////////////////////////////////////////////////////////////////

function modifiers_text(intro, value) {
    if (value == 0) {
        return "";
    }
    let str = intro;
    for (let k in keys.key_modifiers) {
        if ((keys.key_modifiers[k] & value) != 0) {
            str = `${str} ${k}`;
        }
    }
    return str;
}

//////////////////////////////////////////////////////////////////////

const modifier_label = computed(() => {
    if ((current_keycode.value & 0x8000) != 0) {
        return "Consumer control key";
    } else {
        return modifiers_text("Left", current_mod.value & 0xf) + modifiers_text(" Right", (current_mod.value >> 4) & 0xf);
    }
});

//////////////////////////////////////////////////////////////////////

const modifier_class = computed(() => {
    if ((current_keycode.value & 0x8000) != 0) {
        return "dimmer-text";
    } else {
        return "";
    }
});

//////////////////////////////////////////////////////////////////////

function update_modifiers(checked, value) {
    if (checked) {
        current_mod.value |= value;
    } else {
        current_mod.value &= ~value;
    }
}

//////////////////////////////////////////////////////////////////////

</script>

<template>
    <div class="row mt-2 me-0 ms-2">
        <div class="col-8 mt-1 px-0">
            <slot></slot>
        </div>
        <div class="col-4">
        </div>
    </div>
    <div class="input-group limit-height">

        <button class="btn  btn-sm tertiary-bg small-text border form-select ps-2 pe-0 text-start form-control"
            type="button" data-bs-toggle="dropdown">
            <span class="d-inline-block">
                {{ ((current_keycode & 0x8000) == 0) ?
                    keys.hid_keys[current_keycode & 0x7fff] :
                    keys.consumer_control_keys[current_keycode & 0x7fff] }} </span>
        </button>

        <ul class="dropdown-menu border-secondary-subtle rounded-0" style="width:400px">

            <li v-for="key in keys.key_codes" class="w-100">
                <a class="dropdown-item small-text" href="#"
                    @click="current_keycode = (current_keycode & 0xffff0000) | key.keycode | 0">
                    <div class="d-flex flex-row">
                        <div class="w-75 me-0">
                            {{ key.name }}
                        </div>
                        <div class="me-2">
                            {{ key.keycode.toString(16).toUpperCase().padStart(4, '0') }}
                        </div>
                        <span class="w-25 rounded mx-0 px-0 bg-secondary-subtle text-center">
                            Keyboard
                        </span>
                    </div>
                </a>
            </li>
            <li v-for="key in keys.consumer_codes" class="w-100">
                <a class="dropdown-item small-text" href="#"
                    @click="current_keycode = (current_keycode & 0xffff0000) | key.keycode | 0x8000">
                    <div class="d-flex flex-row">
                        <div class="w-75 me-0">
                            {{ key.name }}
                        </div>
                        <div class="me-2">
                            {{ key.keycode.toString(16).toUpperCase().padStart(4, '0') }}
                        </div>
                        <span class="w-25 rounded mx-0 px-0 bg-secondary-subtle text-center">
                            CC Key
                        </span>
                    </div>
                </a>
            </li>
        </ul>
        <button class="btn btn-sm dropdown-toggle border" href="#" role="button" data-bs-toggle="dropdown"
            :disabled="(current_keycode & 0x8000) != 0">
            Modifiers
        </button>
        <form class="dropdown-menu modifiers-dropdown">
            <div class="row mx-3 mt-1">
                <div class="col">
                </div>
                <div class="col text-center">
                    Left
                </div>
                <div class="col text-center">
                    Right
                </div>
            </div>
            <div class="row mx-3" v-for="(val, name) in keys.key_modifiers">
                <div class="col text-end">
                    {{ name }}
                </div>
                <div class="col text-center">
                    <input class="form-check-input" type="checkbox" :checked="(current_mod & val) != 0"
                        @change="update_modifiers($event.target.checked, val)" />
                </div>
                <div class="col text-center">
                    <input class="form-check-input" type="checkbox" :checked="(current_mod & (val << 4)) != 0"
                        @change="update_modifiers($event.target.checked, val << 4)" />
                </div>
            </div>
            <div class="row">
                <div class="col mb-1">
                </div>
            </div>
        </form>

    </div>
    <div class="row">
        <div class="col small-text ms-2 mt-1" :class="modifier_class" style="min-height: 1lh;">
            {{ modifier_label }}
        </div>
    </div>
</template>

<style>
.modifiers-dropdown {
    min-width: 12.75rem;
}

.dimmer-text {
    color: var(--bs-secondary) !important;
}

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
