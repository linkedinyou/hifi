//
//  Texture.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/16/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Texture_h
#define hifi_gpu_Texture_h

#include "Resource.h"
#include <memory>
 
namespace gpu {

class Sampler {
public:

    enum Filter {
        FILTER_MIN_MAG_POINT, // top mip only
        FILTER_MIN_POINT_MAG_LINEAR, // top mip only
        FILTER_MIN_LINEAR_MAG_POINT, // top mip only
        FILTER_MIN_MAG_LINEAR, // top mip only

        FILTER_MIN_MAG_MIP_POINT,
        FILTER_MIN_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_POINT_MAG_MIP_LINEAR,
        FILTER_MIN_LINEAR_MAG_MIP_POINT,
        FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_MAG_MIP_LINEAR,
        FILTER_ANISOTROPIC,

        NUM_FILTERS,
    };

    enum WrapMode {
        WRAP_REPEAT = 0,
        WRAP_MIRROR,
        WRAP_CLAMP,
        WRAP_BORDER,
        WRAP_MIRROR_ONCE,

        NUM_WRAP_MODES
    };

    static const uint8 MAX_MIP_LEVEL = 0xFF;

    class Desc {
    public:
        glm::vec4 _borderColor{ 1.0f };
        uint32 _maxAnisotropy = 16;

        uint8 _wrapModeU = WRAP_REPEAT;
        uint8 _wrapModeV = WRAP_REPEAT;
        uint8 _wrapModeW = WRAP_REPEAT;

        uint8 _filter = FILTER_MIN_MAG_POINT;
        uint8 _comparisonFunc = ALWAYS;
            
        uint8 _mipOffset = 0;
        uint8 _minMip = 0;
        uint8 _maxMip = MAX_MIP_LEVEL;

        Desc() {}
        Desc(const Filter filter) : _filter(filter) {}
    };

    Sampler() {}
    Sampler(const Filter filter) : _desc(filter) {}
    Sampler(const Desc& desc) : _desc(desc) {}
    ~Sampler() {}

    const glm::vec4& getBorderColor() const { return _desc._borderColor; }

    uint32 getMaxAnisotropy() const { return _desc._maxAnisotropy; }

    WrapMode getWrapModeU() const { return WrapMode(_desc._wrapModeU); }
    WrapMode getWrapModeV() const { return WrapMode(_desc._wrapModeV); }
    WrapMode getWrapModeW() const { return WrapMode(_desc._wrapModeW); }

    Filter getFilter() const { return Filter(_desc._filter); }
    ComparisonFunction getComparisonFunction() const { return ComparisonFunction(_desc._comparisonFunc); }
    bool doComparison() const { return getComparisonFunction() != ALWAYS; }

    uint8 getMipOffset() const { return _desc._mipOffset; }
    uint8 getMinMip() const { return _desc._minMip; }
    uint8 getMaxMip() const { return _desc._maxMip; }

protected:
    Desc _desc;
};

class Texture : public Resource {
public:

    class Pixels {
    public:
        Pixels() {}
        Pixels(const Pixels& pixels) = default;
        Pixels(const Element& format, Size size, const Byte* bytes);
        ~Pixels();

        Sysmem _sysmem;
        Element _format;
        bool _isGPULoaded;
    };
    typedef QSharedPointer< Pixels > PixelsPointer;

    class Storage {
    public:
        Storage() {}
        virtual ~Storage() {}
        virtual void reset();
        virtual PixelsPointer editMip(uint16 level);
        virtual const PixelsPointer getMip(uint16 level) const;
        virtual Stamp getStamp(uint16 level) const;
        virtual bool allocateMip(uint16 level);
        virtual bool assignMipData(uint16 level, const Element& format, Size size, const Byte* bytes);
        virtual bool isMipAvailable(uint16 level) const;
        virtual void notifyGPULoaded(uint16 level) const;

    protected:
        Texture* _texture;
        std::vector<PixelsPointer> _mips;

        virtual void assignTexture(Texture* tex);

        friend class Texture;
    };

    enum Type {
        TEX_1D = 0,
        TEX_2D,
        TEX_3D,
        TEX_CUBE,
    };

    static Texture* create1D(const Element& texelFormat, uint16 width, const Sampler& sampler = Sampler());
    static Texture* create2D(const Element& texelFormat, uint16 width, uint16 height, const Sampler& sampler = Sampler());
    static Texture* create3D(const Element& texelFormat, uint16 width, uint16 height, uint16 depth, const Sampler& sampler = Sampler());
    static Texture* createCube(const Element& texelFormat, uint16 width, const Sampler& sampler = Sampler());

    static Texture* createFromStorage(Storage* storage);

    Texture(const Texture& buf); // deep copy of the sysmem texture
    Texture& operator=(const Texture& buf); // deep copy of the sysmem texture
    ~Texture();

    Stamp getStamp() const { return _stamp; }
    Stamp getDataStamp(uint16 level = 0) const { return _storage->getStamp(level); }

    // The size in bytes of data stored in the texture
    Size getSize() const { return _size; }

    // Resize, unless auto mips mode would destroy all the sub mips
    Size resize1D(uint16 width, uint16 numSamples);
    Size resize2D(uint16 width, uint16 height, uint16 numSamples);
    Size resize3D(uint16 width, uint16 height, uint16 depth, uint16 numSamples);
    Size resizeCube(uint16 width, uint16 numSamples);

    // Reformat, unless auto mips mode would destroy all the sub mips
    Size reformat(const Element& texelFormat);

    // Size and format
    Type getType() const { return _type; }

    bool isColorRenderTarget() const;
    bool isDepthStencilRenderTarget() const;

    const Element& getTexelFormat() const { return _texelFormat; }
    bool  hasBorder() const { return false; }

    uint16 getWidth() const { return _width; }
    uint16 getHeight() const { return _height; }
    uint16 getDepth() const { return _depth; }

    uint32 getRowPitch() const { return getWidth() * getTexelFormat().getSize(); }
    uint32 getNumTexels() const { return _width * _height * _depth; }

    uint16 getNumSlices() const { return _numSlices; }
    uint16 getNumSamples() const { return _numSamples; }

    // NumSamples can only have certain values based on the hw
    static uint16 evalNumSamplesUsed(uint16 numSamplesTried);

    // Mips size evaluation

    // The number mips that a dimension could haves
    // = 1 + log2(size)
    static uint16 evalDimNumMips(uint16 size);

    // The number mips that the texture could have if all existed
    // = 1 + log2(max(width, height, depth))
    uint16 evalNumMips() const;

    // Eval the size that the mips level SHOULD have
    // not the one stored in the Texture
    uint16 evalMipWidth(uint16 level) const { return std::max(_width >> level, 1); }
    uint16 evalMipHeight(uint16 level) const { return std::max(_height >> level, 1); }
    uint16 evalMipDepth(uint16 level) const { return std::max(_depth >> level, 1); }
    uint32 evalMipNumTexels(uint16 level) const { return evalMipWidth(level) * evalMipHeight(level) * evalMipDepth(level); }
    uint32 evalMipSize(uint16 level) const { return evalMipNumTexels(level) * getTexelFormat().getSize(); }
    uint32 evalStoredMipSize(uint16 level, const Element& format) const { return evalMipNumTexels(level) * format.getSize(); }

    uint32 evalTotalSize() const {
        uint32 size = 0;
        uint16 minMipLevel = 0;
        uint16 maxMipLevel = maxMip();
        for (uint16 l = minMipLevel; l <= maxMipLevel; l++) {
            size += evalMipSize(l);
        }
        return size * getNumSlices();
    }

    // max mip is in the range [ 1 if no sub mips, log2(max(width, height, depth))]
    // if autoGenerateMip is on => will provide the maxMIp level specified
    // else provide the deepest mip level provided through assignMip
    uint16 maxMip() const;

    // Generate the mips automatically
    // But the sysmem version is not available
    // Only works for the standard formats
    // Specify the maximum Mip level available
    // 0 is the default one
    // 1 is the first level
    // ...
    // nbMips - 1 is the last mip level
    //
    // If -1 then all the mips are generated
    //
    // Return the totalnumber of mips that will be available
    uint16 autoGenerateMips(uint16 maxMip);
    bool isAutogenerateMips() const { return _autoGenerateMips; }

    // Managing Storage and mips

    // Manually allocate the mips down until the specified maxMip
    // this is just allocating the sysmem version of it
    // in case autoGen is on, this doesn't allocate
    // Explicitely assign mip data for a certain level
    // If Bytes is NULL then simply allocate the space so mip sysmem can be accessed
    bool assignStoredMip(uint16 level, const Element& format, Size size, const Byte* bytes);

    // Access the the sub mips
    bool isStoredMipAvailable(uint16 level) const { return _storage->isMipAvailable(level); }
    const PixelsPointer accessStoredMip(uint16 level) const { return _storage->getMip(level); }
    void notifyGPULoaded(uint16 level) const { return _storage->notifyGPULoaded(level); }

    // access sizes for the stored mips
    uint16 getStoredMipWidth(uint16 level) const;
    uint16 getStoredMipHeight(uint16 level) const;
    uint16 getStoredMipDepth(uint16 level) const;
    uint32 getStoredMipNumTexels(uint16 level) const;
    uint32 getStoredMipSize(uint16 level) const;
 
    bool isDefined() const { return _defined; }


    // Own sampler
    void setSampler(const Sampler& sampler);
    const Sampler& getSampler() const { return _sampler; }
    Stamp getSamplerStamp() const { return _samplerStamp; }

protected:
    std::unique_ptr< Storage > _storage;
 
    Stamp _stamp;

    Sampler _sampler;
    Stamp _samplerStamp;


    uint32 _size;
    Element _texelFormat;

    uint16 _width;
    uint16 _height;
    uint16 _depth;

    uint16 _numSamples;
    uint16 _numSlices;

    uint16 _maxMip;
 
    Type _type;
    bool _autoGenerateMips;
    bool _defined;
   
    static Texture* create(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, const Sampler& sampler);
    Texture();

    Size resize(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices);

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef QSharedPointer<Texture> TexturePointer;
typedef std::vector< TexturePointer > Textures;


 // TODO: For now TextureView works with Buffer as a place holder for the Texture.
 // The overall logic should be about the same except that the Texture will be a real GL Texture under the hood
class TextureView {
public:
    typedef Resource::Size Size;

    TexturePointer _texture = TexturePointer(NULL);
    uint16 _subresource = 0;
    Element _element = Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);

    TextureView() {};

    TextureView(const Element& element) :
         _element(element)
    {};

    // create the TextureView and own the Texture
    TextureView(Texture* newTexture, const Element& element) :
        _texture(newTexture),
        _subresource(0),
        _element(element)
    {};
    TextureView(const TexturePointer& texture, uint16 subresource, const Element& element) :
        _texture(texture),
        _subresource(subresource),
        _element(element)
    {};

    TextureView(const TexturePointer& texture, uint16 subresource) :
        _texture(texture),
        _subresource(subresource)
    {};

    ~TextureView() {}
    TextureView(const TextureView& view) = default;
    TextureView& operator=(const TextureView& view) = default;

    explicit operator bool() const { return (_texture); }
    bool operator !() const { return (!_texture); }
};
typedef std::vector<TextureView> TextureViews;

};


#endif
