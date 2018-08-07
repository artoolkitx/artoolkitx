/** Web assembly support */

if(window.artoolkitX_wasm_url) {
    function downloadWasm(url) {
        return new Promise(function(resolve, reject) {
        var wasmXHR = new XMLHttpRequest();
        wasmXHR.open('GET', url, true);
        wasmXHR.responseType = 'arraybuffer';
        wasmXHR.onload = function() { resolve(wasmXHR.response); }
        wasmXHR.onerror = function() { reject('error '  + wasmXHR.status); }
        wasmXHR.send(null);
        });
    }

const wasm = downloadWasm(window.artoolkitX_wasm_url);

    // Module.instantiateWasm is a user-implemented callback which the Emscripten runtime calls to perform
    // the WebAssembly instantiation action. The callback function will be called with two parameters, imports
    // and successCallback. imports is a JS object which contains all the function imports that need to be passed
    // to the Module when instantiating, and once instantiated, the function should call successCallback() with
    // the WebAssembly Instance object.
    // The instantiation can be performed either synchronously or asynchronously. The return value of this function
    // should contain the exports object of the instantiated Module, or an empty dictionary object {} if the
    // instantiation is performed asynchronously, or false if instantiation failed.
    artoolkitXjs.instantiateWasm = function(imports, successCallback) {
        wasm.then(function(wasmBinary) {
            var wasmInstantiate = WebAssembly.instantiate(new Uint8Array(wasmBinary), imports).then(function(output) {
            successCallback(output.instance);
        }).catch(function(e) {
            console.error('wasm instantiation failed! ' + e);
            });
        });
        return {}; // Compiling asynchronously, no exports.
    }
}
