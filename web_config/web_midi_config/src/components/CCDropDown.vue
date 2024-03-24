<!-- CCDropDown - show a list of common CC values in a dropdown with an input -->

<script setup>

import { ref, onMounted, watch, watchEffect } from 'vue';
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

    emit('update:modelValue', current.value);
});

//////////////////////////////////////////////////////////////////////

let matching_cc = ref([]);
let search_text = ref("");

function do_search() {

    matching_cc.value = [];
    if (search_text.value != "") {
        const needle = search_text.value.toUpperCase();
        for (const cc of CC.CCs) {
            if (cc.name.toUpperCase().includes(needle)) {
                matching_cc.value.push(cc);
            }
        }
    }
    if (matching_cc.value.length == 0) {
        Object.assign(matching_cc.value, CC.CCs);
    }
}

onMounted(() => { search_text.value = ""; });

Object.assign(matching_cc.value, CC.CCs);

//////////////////////////////////////////////////////////////////////

watch(search_text, (n) => {
    search_text.value = n;
    do_search();
});

//////////////////////////////////////////////////////////////////////

</script>

<template>
    <div class="input-group">

        <input type="number" class="form-control" v-model.number="current">

        <button class="btn tertiary-bg small-text border rounded-end form-select ps-2 pe-0 text-start" type="button"
            data-bs-toggle="dropdown">
            <slot></slot>
            <span class="d-inline-block"></span>
        </button>

        <ul class="dropdown-menu rounded-0 py-0" style="width:300px">

            <div class="input-group border-bottom">
                <input type="text" v-model="search_text" class="form-control border-0 rounded-0 my-0 pt-1
            shadow-none small-text">
                <button type="button" class="btn bg-transparent py-0 float-end small-text">
                    <i class="btn btn-sm btn-close" @click="search_text = ''; $event.stopPropagation();"></i>
                </button>
            </div>

            <div class="limit-height">
                <li v-for="(cc, index) in matching_cc" class="w-100">
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
            </div>
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

.limit-height {
    max-height: 40vh;
    overflow-y: auto;
}
</style>
