<script setup>
import { ref, onMounted, getCurrentInstance } from 'vue';

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
    <button class='btn btn-sm' @click="toggleTheme">
        <svg width="24px" height="24px" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
            <path d="M12 16C14.2091 16 16 14.2091 16 12C16 9.79086 14.2091 8 12 8V16Z" fill="#808080" />
            <path fill-rule="evenodd" clip-rule="evenodd"
                d="M12 2C6.47715 2 2 6.47715 2 12C2 17.5228 6.47715 22 12 22C17.5228 22 22 17.5228 22 12C22 6.47715 17.5228 2 12 2ZM12 4V8C9.79086 8 8 9.79086 8 12C8 14.2091 9.79086 16 12 16V20C16.4183 20 20 16.4183 20 12C20 7.58172 16.4183 4 12 4Z"
                fill="#808080" />
        </svg>
    </button>
</template>
