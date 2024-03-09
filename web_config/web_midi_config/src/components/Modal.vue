<script setup>

const emit = defineEmits(["update:modelValue"]);

const backdropClick = (event) => {
    if (event.target.id == 'backdrop' && props.closeable == true) {
        emit("update:modelValue", false);
    }
}

const props = defineProps({
    modelValue: {
        type: Boolean,
        required: true,
        default: false
    },
    closeable: {
        type: Boolean,
        required: false,
        default: false
    },
    header: {
        type: String,
        required: false,
        default: null
    },
    container: {
        type: String,
        required: false,
        default: null
    },
    maxwidth: {
        type: String,
        required: false,
        default: null
    }
})

</script>

<template>
    <teleport to="body">
        <transition name="fade">
            <div v-if="modelValue" class="pt-4 position-fixed top-0 start-0 h-100 w-100 z-2"
                style="background-color: rgba(0, 0, 0, 0.8)">
                <div id="backdrop" @click="backdropClick"
                    class="modal1-dialog h-75 overflow-auto align-items-center d-flex z3">
                    <div class="card px-0" :class="(container == null) ? 'container' : `container-${container}`"
                        :style="(maxwidth != null) ? `max-width: ${maxwidth}` : null">
                        <div class="card-header h5 text-center py-2">
                            <span>
                                {{ header }}
                            </span>
                            <span class="position-absolute top-0 end-0 my-2 mx-2">
                                <button v-if="closeable" @click='$emit("update:modelValue", false)'
                                    class="btn btn-text btn-close float-end"></button>
                            </span>
                        </div>
                        <div class="card-body">
                            <slot></slot>
                        </div>
                    </div>
                </div>
            </div>
        </transition>
    </teleport>
</template>

<style scoped>
.fade-enter-active,
.fade-leave-active {
    transition: opacity 0.2s
}

.fade-enter-from,
.fade-leave-to {
    opacity: 0
}
</style>