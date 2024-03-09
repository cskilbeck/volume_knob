<!-- CCDropDown - show a list of common CC values in a dropdown with an input -->

<script setup>

import { ref, watch, watchEffect } from 'vue';
import CC from '../CC.js';

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
    current.value = Math.max(0, Math.min(Number(n), 127));
});

</script>

<template>
    <div class="input-group limit-height">

        <input type="number" class="form-control" v-model.number="current">

        <button class="btn tertiary-bg small-text border rounded-end" type="button" data-bs-toggle="dropdown">
            <slot></slot>
        </button>

        <ul class="dropdown-menu border-secondary-subtle rounded-0" style="width:300px">

            <li v-for="(cc, index) in CC.CCs" class="w-100">

                <a class="dropdown-item small-text" href="#" @click="current = index">
                    <div class="d-flex flex-row">
                        <div class="w-75">
                            {{ cc.name }}
                        </div>
                        <div class="w-25 text-end me-4">
                            {{ index.toString() }}
                        </div>
                        <div class="w-25 text-end">
                            <span class="rounded px-2 pb-1"
                                :class='(CC.is_MSB(cc) || CC.is_LSB(cc) ? "bg-secondary-subtle" : "") + " " + (CC.is_MSB(cc) ? "bg-secondary-subtle" : "text-secondary")'>
                                {{ CC.is_MSB(cc) ? "EXT" : CC.is_LSB(cc) ? "LSB" : "" }}
                            </span>
                        </div>
                    </div>
                </a>

            </li>
        </ul>
    </div>
    <div class="row mb-2 me-0 small-text text-secondary">
        <div class="col pe-0">
            {{ `${CC.CCs[current].name} ${CC.is_LSB(CC.CCs[current]) ? "LSB" : ""}` }}
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
