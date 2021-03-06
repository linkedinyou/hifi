//
//  GLBackendPipeline.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

#include "Format.h"

using namespace gpu;

GLBackend::GLPipeline::GLPipeline() :
    _program(nullptr),
    _state(nullptr)
{}

GLBackend::GLPipeline::~GLPipeline() {
    _program = nullptr;
    _state = nullptr;
}

GLBackend::GLPipeline* GLBackend::syncGPUObject(const Pipeline& pipeline) {
    GLPipeline* object = Backend::getGPUObject<GLBackend::GLPipeline>(pipeline);

    // If GPU object already created then good
    if (object) {
        return object;
    }

    // No object allocated yet, let's see if it's worth it...
    ShaderPointer shader = pipeline.getProgram();
    GLShader* programObject = GLBackend::syncGPUObject((*shader));
    if (programObject == nullptr) {
        return nullptr;
    }

    StatePointer state = pipeline.getState();
    GLState* stateObject = GLBackend::syncGPUObject((*state));
    if (stateObject == nullptr) {
        return nullptr;
    }

    // Program and state are valid, we can create the pipeline object
    if (!object) {
        object = new GLPipeline();
        Backend::setGPUObject(pipeline, object);
    }

    object->_program = programObject;
    object->_state = stateObject;

    return object;
}

void GLBackend::do_setPipeline(Batch& batch, uint32 paramOffset) {
    PipelinePointer pipeline = batch._pipelines.get(batch._params[paramOffset + 0]._uint);

    if (_pipeline._pipeline == pipeline) {
        return;
    }
   
    if (_pipeline._needStateSync) {
        syncPipelineStateCache();
        _pipeline._needStateSync = false;
    }

    // null pipeline == reset
    if (!pipeline) {
        _pipeline._pipeline.reset();

        _pipeline._program = 0;
        _pipeline._invalidProgram = true;

        _pipeline._state = nullptr;
        _pipeline._invalidState = true;
    } else {
        auto pipelineObject = syncGPUObject((*pipeline));
        if (!pipelineObject) {
            return;
        }

        // check the program cache
        if (_pipeline._program != pipelineObject->_program->_program) {
            _pipeline._program = pipelineObject->_program->_program;
            _pipeline._invalidProgram = true;
        }

        // Now for the state
        if (_pipeline._state != pipelineObject->_state) {
            _pipeline._state = pipelineObject->_state;
            _pipeline._invalidState = true;
        }

        // Remember the new pipeline
        _pipeline._pipeline = pipeline;
    }

    // THis should be done on Pipeline::update...
    if (_pipeline._invalidProgram) {
        glUseProgram(_pipeline._program);
        (void) CHECK_GL_ERROR();
        _pipeline._invalidProgram = false;
    }
}

#define DEBUG_GLSTATE
void GLBackend::updatePipeline() {
#ifdef DEBUG_GLSTATE
    if (_pipeline._needStateSync) {
         State::Data state;
         getCurrentGLState(state);
         State::Signature signature = State::evalSignature(state);
         (void) signature; // quiet compiler
    }
#endif

    if (_pipeline._invalidProgram) {
        // doing it here is aproblem for calls to glUniform.... so will do it on assing...
        glUseProgram(_pipeline._program);
        (void) CHECK_GL_ERROR();
        _pipeline._invalidProgram = false;
    }

    if (_pipeline._invalidState) {
        if (_pipeline._state) {
            // first reset to default what should be
            // the fields which were not to default and are default now
            resetPipelineState(_pipeline._state->_signature);
            
            // Update the signature cache with what's going to be touched
            _pipeline._stateSignatureCache |= _pipeline._state->_signature;

            // And perform
            for (auto command: _pipeline._state->_commands) {
                command->run(this);
            }
        } else {
            // No state ? anyway just reset everything
            resetPipelineState(0);
        }
        _pipeline._invalidState = false;
    }
}

void GLBackend::do_setUniformBuffer(Batch& batch, uint32 paramOffset) {
    GLuint slot = batch._params[paramOffset + 3]._uint;
    BufferPointer uniformBuffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    GLintptr rangeStart = batch._params[paramOffset + 1]._uint;
    GLsizeiptr rangeSize = batch._params[paramOffset + 0]._uint;

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    GLuint bo = getBufferID(*uniformBuffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, slot, bo, rangeStart, rangeSize);
#else
    GLfloat* data = (GLfloat*) (uniformBuffer->getData() + rangeStart);
    glUniform4fv(slot, rangeSize / sizeof(GLfloat[4]), data);
 
    // NOT working so we ll stick to the uniform float array until we move to core profile
    // GLuint bo = getBufferID(*uniformBuffer);
    //glUniformBufferEXT(_shader._program, slot, bo);
#endif
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_setUniformTexture(Batch& batch, uint32 paramOffset) {
    GLuint slot = batch._params[paramOffset + 1]._uint;
    TexturePointer uniformTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);

    GLuint to = getTextureID(uniformTexture);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, to);

    (void) CHECK_GL_ERROR();
}

