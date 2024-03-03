<!-- CCDropDown - show a list of common CC values in a dropdown with an input -->

<script setup>

import { watch } from 'vue';
import cc_names from '../cc_names.js';

const model = defineModel();

const props = defineProps({
    label: {
        type: String,
        required: true
    },
    hidden: {
        type: Boolean,
        required: false
    }
});

function constrainTo7Bits(event) {
    model.value = Math.max(0, Math.min(Number(event.target.value), 127));
}

</script>

<template>
    <div :class="{ hide: hidden }">
        <div class="input-group mb-1 limit-height">

            <input type="number" class="form-control" v-model="model" @input="constrainTo7Bits">

            <button class="btn tertiary-bg small-text border rounded-end" type="button" data-bs-toggle="dropdown">{{
        label }}</button>

            <ul class="dropdown-menu border-secondary-subtle rounded-0">

                <li v-for="[id, ccname] in cc_names.cc_names_map">

                    <a class="dropdown-item small-text py-0" href="#" @click="model = id">
                        {{ id.toString().padStart(3, '0') }}: {{ ccname }}
                    </a>

                </li>
            </ul>
        </div>
        <div class="row mx-1 small-text text-success">{{ cc_names.get_cc_name(model) }}</div>
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
