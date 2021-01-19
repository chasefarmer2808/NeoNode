function docReady(fn) {
    // see if DOM is already available
    if (document.readyState === "complete" || document.readyState === "interactive") {
        // call on next available tick
        setTimeout(fn, 1);
    } else {
        document.addEventListener("DOMContentLoaded", fn);
    }
}    

docReady(function() {
    const URL = window.location.href;
    const colorPicker = document.getElementById('color-picker');

    colorPicker.addEventListener('change', fill, false)
    
    function fill(event) {
        const {r, g, b} = hexToRgb(event.target.value.replace('#', ''));
        console.log(r, g, b);
        postFill(r, g, b).then(data => {
            console.log(data);
        })
    }

    function hexToRgb(hex) {
        var bigint = parseInt(hex, 16);
        var r = (bigint >> 16) & 255;
        var g = (bigint >> 8) & 255;
        var b = bigint & 255;

        return {r, g, b};
    }
    
    async function postFill(r, g, b) {
        const formData = new FormData();
        formData.append("r", r);
        formData.append("g", g);
        formData.append("b", b);
    
        const options = {
            method: 'POST',
            body: formData,
            redirect: 'follow'
        };
        const colorUrl = `${URL}color`;
        console.log(colorUrl);
        const response = await fetch(colorUrl, options);
    
        return response;
    }
});
