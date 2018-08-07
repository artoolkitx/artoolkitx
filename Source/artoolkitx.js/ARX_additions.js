//NOTE: As to my knowlege every JS code that is processed by Emscripten needs to be ES5 compatible. Meaning no const/let or arrow (=>) functions

// Many thanks to https://github.com/Planeshifter for his project: https://github.com/Planeshifter/emscripten-examples/tree/master/01_PassingArrays
function _arrayToHeap(typedArray) {
    var numBytes = typedArray.length * typedArray.BYTES_PER_ELEMENT;
    var ptr = Module._malloc(numBytes);
    var heapBytes = new Uint8Array(Module.HEAPU8.buffer, ptr, numBytes);
    heapBytes.set(new Uint8Array(typedArray.buffer));
    return heapBytes;
}

Module["getProjectionMatrix"] = function(nearPlane, farPlane) {
    var projectionMatrix = new Float64Array(16);
    var heapBytes = _arrayToHeap(projectionMatrix);

    if( ! Module.ccall('arwGetProjectionMatrix', 'boolean',['number','number','number'], [nearPlane, farPlane, heapBytes.byteOffset])) {
        return undefined;
    }
    var returnValue = new Float64Array(heapBytes);
    Module._free(heapBytes.byteOffset);

    return returnValue;
};

Module["getTrackablePatternConfig"] = function (trackableId, patternID) {
    var heapBytes = _arrayToHeap(new Float64Array(16));
    var widthHeapBytes = _arrayToHeap(new Float64Array(1));
    var heightHeapBytes = _arrayToHeap(new Float64Array(1));
    var sizeXHeapBytes = _arrayToHeap(new Float64Array(1));
    var sizeYHeapBytes = _arrayToHeap(new Float64Array(1));
    if( !Module.ccall('arwGetTrackablePatternConfig','boolean',['number','number','number','number','number','number','number','number'], [trackableId, patternID, heapBytes.byteOffset, widthHeapBytes.byteOffset, heightHeapBytes.byteOffset, sizeXHeapBytes.byteOffset, sizeYHeapBytes.byteOffset])){
        return undefined;
    }

    var returnObject = {
        matrix: new Float64Array(heapBytes.buffer, heapBytes.byteOffset, 16),
        width: new Float64Array(widthHeapBytes.buffer, widthHeapBytes.byteOffset, 1)[0],
        height: new Float64Array(heightHeapBytes.buffer, heightHeapBytes.byteOffset, 1)[0],
        sizeX: new Float64Array(sizeXHeapBytes.buffer, sizeXHeapBytes.byteOffset, 1)[0],
        sizeY: new Float64Array(sizeYHeapBytes.buffer, sizeYHeapBytes.byteOffset, 1)[0]
    };
    Module._free(heapBytes.byteOffset);
    Module._free(widthHeapBytes.byteOffset);
    Module._free(heightHeapBytes.byteOffset);
    Module._free(sizeXHeapBytes.byteOffset);
    Module._free(sizeYHeapBytes.byteOffset);

    return returnObject;
}
    
Module["getTrackablePatternImage"] = function (trackableId, patternID) {

    //Read trackable pattern config to get the size of the trackable. This is needed to define the Array size.
    var trackableConfig= Module.getTrackablePatternConfig(trackableId, patternID);
    if(trackableConfig) {
        var imageBuffer = _arrayToHeap(new Uint32Array(trackableConfig.width * trackableConfig.height));

        if ( ! Module.ccall('arwGetTrackablePatternImage', 'boolean', ['number','number','number'], [trackableId, patternID, imageBuffer.byteOffset]) ) {
            return undefined;
        }
        imageBuffer = Uint32Array(imageBuffer.buffer, imageBuffer.byteOffset, trackableConfig.width * trackableConfig.height);
        Module._free(imageBuffer.byteOffset);
        return imageBuffer;
    }
}

Module["loadOpticalParams"] = function(opticalParamName, opticalParamBuffer, projectionNearPlane, projectionFarPlane) {

    var opticalParamBufferLength = 0;
    var opticalParamBufferHeap;
    if(!opticalParamName) {
        var opticalParamBufferHeap = _arrayToHeap(opticalParamBuffer);
        var opticalParamBufferLength = opticalParamBuffer.length;
    }

    var fovHeap = _arrayToHeap(new Float64Array(1));
    var aspectHeap = _arrayToHeap(new Float64Array(1));
    var transformationMatrixHeap = _arrayToHeap(new Float64Array(16));
    var perspectiveMatrixHeap = _arrayToHeap(new Float64Array(16));

    Module.ccall('arwLoadOpticalParams', 'boolean', ['string', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'], [opticalParamName, opticalParamBufferHeap.byteOffset, opticalParamBufferLength, projectionNearPlane, projectionFarPlane, fovHeap.byteOffset, aspectHeap.byteOffset, transformationMatrixHeap.byteOffset, perspectiveMatrixHeap.byteOffset]);

    var returnObject = {
        fov: new Float64Array(fovHeap.buffer, fovHeap.byteOffset, 1)[0],
        aspect: new Float64Array(aspectHeap.buffer, aspectHeap.byteOffset, 1)[0],
        opticalParamsTransMatrix: new Float64Array(transformationMatrixHeap.buffer, transformationMatrixHeap.byteOffset, 16),
        perspectiveMatrix: new Float64Array(perspectiveMatrixHeap.buffer, perspectiveMatrixHeap.byteOffset, 16)
    }

    Module._free(fovHeap.byteOffset);
    Module._free(aspectHeap.byteOffset);
    Module._free(transformationMatrixHeap.byteOffset);
    Module._free(perspectiveMatrixHeap.byteOffset);
    return returnObject;
};

Module["onRuntimeInitialized"] = function() {
    var event = new Event('artoolkitX-loaded');
    window.dispatchEvent(event);
}