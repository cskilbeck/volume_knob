//////////////////////////////////////////////////////////////////////
// Shared logic composable for HIDDevice_1 and HIDDevice_2.
// options.supports_mouse controls whether mouse event types are shown
// in the key dropdowns.

import { toRaw, watch, ref, nextTick, onMounted } from 'vue'
import hid from './hid.js'

export function useHIDDevice(props, { supports_mouse = false } = {}) {

    //////////////////////////////////////////////////////////////////////
    // modal visibility flags

    const flashModal = ref(false);
    const pasteConfigModal = ref(false);
    const errorModal = ref(false);

    let config_paste_textarea_contents = ref("");
    let error_messages = ref([]);

    //////////////////////////////////////////////////////////////////////
    // per-device label stored in localStorage

    function cookie_name() {
        return `${props.device.name}_label`;
    }

    let device_label = ref(localStorage.getItem(cookie_name()) || "Unnamed");

    function save_name() {
        if (device_label.value.length === 0) {
            device_label.value = "Unnamed";
        }
        localStorage.setItem(cookie_name(), device_label.value);
    }

    //////////////////////////////////////////////////////////////////////

    let config_changed = ref(false);
    let stored_config = null;
    let collapsed = ref(false);

    //////////////////////////////////////////////////////////////////////
    // noddy shallow equality for config objects

    function shallowEqual(object1, object2) {
        if (object1 == null || object2 == null) {
            return false;
        }
        const keys1 = Object.keys(object1);
        const keys2 = Object.keys(object2);
        if (keys1.length !== keys2.length) {
            return false;
        }
        for (let key of keys1) {
            if (object1[key] !== object2[key]) {
                return false;
            }
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////
    // convert config object → ui state

    function ui_from_config(config) {

        let errors = [];

        for (const field in config) {
            if (hid.default_config[field] === undefined) {
                errors.push(`Unknown field: ${field}`);
            }
        }

        for (const field in hid.default_config) {
            if (config[field] === undefined) {
                errors.push(`Missing field: ${field}`);
            } else {
                const cfg_type = typeof config[field];
                const default_type = typeof hid.default_config[field];
                if (cfg_type !== default_type) {
                    errors.push(`Field [${field}] is ${cfg_type}, should be ${default_type}`);
                }
            }
        }

        if (errors.length !== 0) {
            error_messages.value = errors;
            return null;
        }

        return {
            config_version: config.config_version,

            key_clockwise: config.key_clockwise,
            key_counterclockwise: config.key_counterclockwise,
            key_press: config.key_press,

            mod_clockwise: config.mod_clockwise,
            mod_counterclockwise: config.mod_counterclockwise,
            mod_press: config.mod_press,

            cf_led_flash_on_cw: (config.flags & hid.flags.cf_led_flash_on_cw) !== 0,
            cf_led_flash_on_ccw: (config.flags & hid.flags.cf_led_flash_on_ccw) !== 0,
            cf_led_flash_on_press: (config.flags & hid.flags.cf_led_flash_on_press) !== 0,
            cf_led_flash_on_release: (config.flags & hid.flags.cf_led_flash_on_release) !== 0,
            cf_reverse_rotation: (config.flags & hid.flags.cf_reverse_rotation) !== 0,
        };
    }

    //////////////////////////////////////////////////////////////////////
    // convert ui state → config object

    function config_from_ui() {
        return {
            config_version: ui.value.config_version,

            key_clockwise: ui.value.key_clockwise,
            key_counterclockwise: ui.value.key_counterclockwise,
            key_press: ui.value.key_press,

            mod_clockwise: ui.value.mod_clockwise,
            mod_counterclockwise: ui.value.mod_counterclockwise,
            mod_press: ui.value.mod_press,

            flags: (ui.value.cf_led_flash_on_cw ? hid.flags.cf_led_flash_on_cw : 0)
                | (ui.value.cf_led_flash_on_ccw ? hid.flags.cf_led_flash_on_ccw : 0)
                | (ui.value.cf_led_flash_on_press ? hid.flags.cf_led_flash_on_press : 0)
                | (ui.value.cf_led_flash_on_release ? hid.flags.cf_led_flash_on_release : 0)
                | (ui.value.cf_reverse_rotation ? hid.flags.cf_reverse_rotation : 0)
        };
    }

    //////////////////////////////////////////////////////////////////////
    // initialise ui from default config (error_messages must be declared first)

    let ui = ref(ui_from_config(hid.default_config));

    //////////////////////////////////////////////////////////////////////
    // watch for ui changes and update config_changed

    watch(() => { return ui; },
        () => {
            config_changed.value = !shallowEqual(config_from_ui(), stored_config);
        },
        { deep: true }
    );

    //////////////////////////////////////////////////////////////////////
    // apply a config object to the ui (used by on_config_loaded and onMounted)

    function apply_config(config) {
        stored_config = Object.assign({}, toRaw(config));
        let new_ui = ui_from_config(config);
        if (!new_ui) {
            console.log("No config, using default");
            new_ui = Object.assign({}, ui_from_config(hid.default_config));
        }
        ui.value = new_ui;
        nextTick(() => {
            config_changed.value = false;
        });
    }

    //////////////////////////////////////////////////////////////////////
    // callbacks set on the device object so hid.js can notify us

    props.device.on_config_loaded = () => {
        apply_config(props.device.config);
    };

    props.device.on_config_saved = () => {
        console.log("Save config");
        stored_config = Object.assign({}, toRaw(props.device.config));
        config_changed.value = false;
    };

    //////////////////////////////////////////////////////////////////////
    // if the version response arrived before this component mounted,
    // the config will already be in device.config — apply it now

    onMounted(() => {
        if (props.device.config_loaded) {
            apply_config(props.device.config);
        }
    });

    //////////////////////////////////////////////////////////////////////

    function toggle_expand() {
        collapsed.value = !collapsed.value;
    }

    function store_config() {
        console.log("Store config");
        props.device.config = config_from_ui();
        hid.set_config(props.device);
    }

    function json_from_config(config) {
        return JSON.stringify(config, null, 4);
    }

    function reset_to_defaults() {
        console.log("reset_to_defaults");
        Object.assign(props.device.config, hid.default_config);
        ui.value = ui_from_config(props.device.config);
    }

    function copy_config() {
        const config = config_from_ui();
        const json = json_from_config(config);
        navigator.clipboard.writeText(json);
    }

    function show_paste_dialog() {
        config_paste_textarea_contents.value = "";
        pasteConfigModal.value = true;
    }

    function paste_config(json_text) {
        let new_config = {};
        try {
            new_config = JSON.parse(json_text);
        } catch (err) {
            error_messages.value = [err.message];
            errorModal.value = true;
            return;
        }
        let new_ui = ui_from_config(new_config);
        if (new_ui == null) {
            errorModal.value = true;
        } else {
            ui.value = new_ui;
        }
    }

    function rotation_matrix(cx, cy, angle) {
        let a = angle * 3.14159265 / 180;
        let c = Math.cos(a);
        let s = Math.sin(a);
        return `matrix(${c}, ${s}, ${-s}, ${c}, ${cx * (1 - c) + cy * s}, ${cy * (1 - c) - cx * s})`;
    }

    //////////////////////////////////////////////////////////////////////

    return {
        flashModal,
        pasteConfigModal,
        errorModal,
        config_paste_textarea_contents,
        error_messages,
        device_label,
        save_name,
        config_changed,
        ui,
        collapsed,
        toggle_expand,
        store_config,
        json_from_config,
        config_from_ui,
        reset_to_defaults,
        copy_config,
        show_paste_dialog,
        paste_config,
        rotation_matrix,
    };
}
