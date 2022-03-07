#pragma once
#import <Metal/Metal.h>
#include <vulkan/vulkan.hpp>

// MoltenVK/include/MoltenVK/mvk_datatypes.h

/** Enumerates the data type of a format. */
typedef enum {
    kMVKFormatNone,             /**< Format type is unknown. */
    kMVKFormatColorHalf,        /**< A 16-bit floating point color. */
    kMVKFormatColorFloat,       /**< A 32-bit floating point color. */
    kMVKFormatColorInt8,        /**< A signed 8-bit integer color. */
    kMVKFormatColorUInt8,       /**< An unsigned 8-bit integer color. */
    kMVKFormatColorInt16,       /**< A signed 16-bit integer color. */
    kMVKFormatColorUInt16,      /**< An unsigned 16-bit integer color. */
    kMVKFormatColorInt32,       /**< A signed 32-bit integer color. */
    kMVKFormatColorUInt32,      /**< An unsigned 32-bit integer color. */
    kMVKFormatDepthStencil,     /**< A depth and stencil value. */
    kMVKFormatCompressed,       /**< A block-compressed color. */
} MVKFormatType;

// MoltenVK/MoltenVK/API/vk_mvk_moltenvk.h

/** Identifies the type of rounding Metal uses for float to integer conversions in particular calculatons. */
typedef enum MVKFloatRounding {
    MVK_FLOAT_ROUNDING_NEAREST     = 0,  /**< Metal rounds to nearest. */
    MVK_FLOAT_ROUNDING_UP          = 1,  /**< Metal rounds towards positive infinity. */
    MVK_FLOAT_ROUNDING_DOWN        = 2,  /**< Metal rounds towards negative infinity. */
    MVK_FLOAT_ROUNDING_UP_MAX_ENUM = 0x7FFFFFFF
} MVKFloatRounding;

// MoltenVK/MoltenVK/Utility/MVKFoundation.h

/** Returns the result of a division, rounded up. */
template<typename T, typename U>
constexpr typename std::common_type<T, U>::type mvkCeilingDivide(T numerator, U denominator) {
    typedef typename std::common_type<T, U>::type R;
    // Short circuit very common usecase of dividing by one.
    return (denominator == 1) ? numerator : (R(numerator) + denominator - 1) / denominator;
}

/**
 * If pVal is not null, clears the memory occupied by *pVal by writing zeros to all bytes.
 * The optional count allows clearing multiple elements in an array.
 */
template<typename T>
void mvkClear(T* pVal, size_t count = 1) { if (pVal) { memset(pVal, 0, sizeof(T) * count); } }

/**
 * If pVal is not null, overrides the const declaration, and clears the memory occupied by *pVal
 * by writing zeros to all bytes. The optional count allows clearing multiple elements in an array.
*/
template<typename T>
void mvkClear(const T* pVal, size_t count = 1) { mvkClear((T*)pVal, count); }

/** Selects and returns one of the values, based on the platform OS. */
template<typename T>
const T& mvkSelectPlatformValue(const T& macOSVal, const T& iOSVal) {
#if MVK_IOS_OR_TVOS
    return iOSVal;
#endif
#if MVK_MACOS
    return macOSVal;
#endif
}

/** Enables the flags (sets bits to 1) within the value parameter specified by the bitMask parameter. */
template<typename Tv, typename Tm>
void mvkEnableFlags(Tv& value, const Tm bitMask) { value = (Tv)(value | bitMask); }

/** Disables the flags (sets bits to 0) within the value parameter specified by the bitMask parameter. */
template<typename Tv, typename Tm>
void mvkDisableFlags(Tv& value, const Tm bitMask) { value = (Tv)(value & ~(Tv)bitMask); }

/** Returns whether the specified value has ANY of the flags specified in bitMask enabled (set to 1). */
template<typename Tv, typename Tm>
bool mvkIsAnyFlagEnabled(Tv value, const Tm bitMask) { return ((value & bitMask) != 0); }

/** Returns whether the specified value has ALL of the flags specified in bitMask enabled (set to 1). */
template<typename Tv, typename Tm>
bool mvkAreAllFlagsEnabled(Tv value, const Tm bitMask) { return ((value & bitMask) == bitMask); }

/** Returns whether the specified value has ONLY one or more of the flags specified in bitMask enabled (set to 1), and none others. */
template<typename Tv, typename Tm>
bool mvkIsOnlyAnyFlagEnabled(Tv value, const Tm bitMask) { return (mvkIsAnyFlagEnabled(value, bitMask) && ((value | bitMask) == bitMask)); }

/** Returns whether the specified value has ONLY ALL of the flags specified in bitMask enabled (set to 1), and none others. */
template<typename Tv, typename Tm>
bool mvkAreOnlyAllFlagsEnabled(Tv value, const Tm bitMask) { return (value == bitMask); }

// Common/MVKOSExtensions.h

typedef float MVKOSVersion;

/** Returns a MVKOSVersion built from the version components. */
inline MVKOSVersion mvkMakeOSVersion(uint32_t major, uint32_t minor, uint32_t patch) {
    return (float)major + ((float)minor / 100.0f) + ((float)patch / 10000.0f);
}

/**
 * Returns the operating system version as an MVKOSVersion, which is a float in which the
 * whole number portion indicates the major version, and the fractional portion indicates 
 * the minor and patch versions, associating 2 decimal places to each.
 * - (10.12.3) => 10.1203
 * - (8.0.2) => 8.0002
 */
inline MVKOSVersion mvkOSVersion() {
    static MVKOSVersion _mvkOSVersion = 0;
    if ( !_mvkOSVersion ) {
        NSOperatingSystemVersion osVer = [[NSProcessInfo processInfo] operatingSystemVersion];
        _mvkOSVersion = mvkMakeOSVersion((uint32_t)osVer.majorVersion, (uint32_t)osVer.minorVersion, (uint32_t)osVer.patchVersion);
    }
    return _mvkOSVersion;
}

/** Returns whether the operating system version is at least minVer. */
inline bool mvkOSVersionIsAtLeast(MVKOSVersion minVer) { return mvkOSVersion() >= minVer; }

// Manual hacks

#ifdef NS_BLOCK_ASSERTIONS
#   define MVK_BLOCK_ASSERTIONS     1
#else
#   define MVK_BLOCK_ASSERTIONS     0
#endif

#define MVKAssert(test, ...)                    \
do {                                            \
    assert((test) || MVK_BLOCK_ASSERTIONS);   \
} while(0)

struct MVKPhysicalDeviceMetalFeatures
{
    VkBool32 stencilViews;                          /**< If true, stencil aspect views are supported through the MTLPixelFormatX24_Stencil8 and MTLPixelFormatX32_Stencil8 formats. */
    VkBool32 renderLinearTextures;                  /**< If true, linear textures are renderable. */
    MVKFloatRounding clearColorFloatRounding;       /**< Identifies the type of rounding Metal uses for MTLClearColor float to integer conversions. */
};

class MVKVulkanAPIObject;

class MVKBaseObject {
public:
    virtual ~MVKBaseObject() = default;

    /** Returns the Vulkan API opaque object controlling this object. */
    virtual MVKVulkanAPIObject* getVulkanAPIObject() = 0;

    /**
     * Report a Vulkan error message, on behalf of the object. which may be nil.
     * Reporting includes logging to a standard system logging stream, and if the object
     * is not nil and has access to the VkInstance, the message will also be forwarded
     * to the VkInstance for output to the Vulkan debug report messaging API.
     */
    static VkResult reportError(MVKBaseObject* mvkObj, VkResult vkErr, const char* format, ...) __printflike(3, 4) {
        MVKAssert(false);
        return vkErr;
    }
};

class MVKVulkanAPIObject : public MVKBaseObject {
public:
    virtual ~MVKVulkanAPIObject() = default;
};

class MVKPhysicalDevice : public MVKVulkanAPIObject {
public:
    virtual ~MVKPhysicalDevice() = default;

    /** Returns the underlying Metal device. */
    virtual id<MTLDevice> getMTLDevice() = 0;

    /** Populates the specified structure with the Metal-specific features of this device. */
    virtual const MVKPhysicalDeviceMetalFeatures* getMetalFeatures() = 0;
};
