<script setup>
import { ref, onMounted } from 'vue';

const setTheme = (theme) => {
    localStorage.setItem('user-theme', theme);
    userTheme.value = theme;
    document.documentElement.setAttribute('data-bs-theme', theme);
};

const getTheme = () => {
    return localStorage.getItem('user-theme');
};

const toggleTheme = () => {
    const activeTheme = localStorage.getItem('user-theme');
    if (activeTheme === 'light') {
        setTheme('dark');
    } else {
        setTheme('light');
    }
};

const getMediaPreference = () => {
    const hasDarkPreference = window.matchMedia('(prefers-color-scheme: dark)').matches;
    if (hasDarkPreference) {
        return 'dark';
    } else {
        return 'light';
    }
};

const userTheme = ref(getTheme() || getMediaPreference());

onMounted(() => setTheme(userTheme.value));
</script>

<template>
    <button class='btn btn-sm mx-2' @click="toggleTheme">
        <svg version="1.1" width="24" height="24" viewBox="0 0 256 256" xml:space="preserve">
            <g style="stroke: none; stroke-width: 0; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: none; fill-rule: nonzero; opacity: 1;"
                transform="translate(1.4065934065934016 1.4065934065934016) scale(2.81 2.81)">
                <rect x="42" y="0" rx="0" ry="0" width="6" height="15.79" style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10;
                    fill: currentColor; fill-rule: nonzero; opacity: 1;" transform=" matrix(1 0 0 1 0 0) " />
                <rect x="42" y="74.21" rx="0" ry="0" width="6" height="15.79"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(1 0 0 1 0 0) " />
                <rect x="0" y="42" rx="0" ry="0" width="15.79" height="6"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(1 0 0 1 0 0) " />
                <rect x="74.21" y="42" rx="0" ry="0" width="15.79" height="6"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(1 0 0 1 0 0) " />
                <path
                    d="M 74.698 11.059 l -15.71 15.71 C 54.99 23.689 50.129 22 45 22 c -12.682 0 -23 10.318 -23 23 c 0 5.13 1.689 9.991 4.769 13.989 L 11.059 74.698 l 4.242 4.242 L 78.94 15.301 L 74.698 11.059 z"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(1 0 0 1 0 0) " stroke-linecap="round" />
                <rect x="15.76" y="10.87" rx="0" ry="0" width="6" height="15.79"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(0.7071 -0.7071 0.7071 0.7071 -7.7721 18.7634) " />
                <rect x="68.24" y="63.34" rx="0" ry="0" width="6" height="15.79"
                    style="stroke: none; stroke-width: 1; stroke-dasharray: none; stroke-linecap: butt; stroke-linejoin: miter; stroke-miterlimit: 10; fill: currentColor; fill-rule: nonzero; opacity: 1;"
                    transform=" matrix(0.7071 -0.7071 0.7071 0.7071 -29.5071 71.2363) " />
            </g>
        </svg>
    </button>
</template>
