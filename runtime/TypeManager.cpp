/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "TypeManager"

#include "TypeManager.h"
#include "test/TmpDirectoryUtils.h"

#include <LegacyUtils.h>
#include <android-base/file.h>
#include <android-base/properties.h>

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
#ifdef __ANDROID__
#include <PackageInfo.h>
#include <procpartition/procpartition.h>
#endif  // __ANDROID__

#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

namespace android {
namespace nn {

// Replacement function for std::string_view::starts_with()
// which shall be available in C++20.
#if __cplusplus >= 202000L
#error "When upgrading to C++20, remove this error and file a bug to remove this workaround."
#endif
inline bool StartsWith(std::string_view sv, std::string_view prefix) {
    return sv.substr(0u, prefix.size()) == prefix;
}

namespace {

constexpr uint32_t kMaxPrefix = (1 << kExtensionPrefixBits) - 1;

// Checks if the two structures contain the same information. The order of
// operand types within the structures does not matter.
bool equal(const Extension& a, const Extension& b) {
    NN_RET_CHECK_EQ(a.name, b.name);
    // Relies on the fact that TypeManager sorts operandTypes.
    NN_RET_CHECK(a.operandTypes == b.operandTypes);
    return true;
}

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD

// Property for disabling NNAPI vendor extensions on product image (used on GSI /product image,
// which can't use NNAPI vendor extensions).
const char kVExtProductDeny[] = "ro.nnapi.extensions.deny_on_product";
bool isNNAPIVendorExtensionsUseAllowedInProductImage() {
    const std::string vExtProductDeny = android::base::GetProperty(kVExtProductDeny, "");
    return vExtProductDeny.empty();
}

// The file containing the list of Android apps and binaries allowed to use vendor extensions.
// Each line of the file contains new entry. If entry is prefixed by
// '/' slash, then it's a native binary path (e.g. '/data/foo'). If not, it's a name
// of Android app package (e.g. 'com.foo.bar').
const char kAppAllowlistPath[] = "/vendor/etc/nnapi_extensions_app_allowlist";
const char kCtsAllowlist[] = NN_TMP_DIR "/CTSNNAPITestCases";
std::vector<std::string> getVendorExtensionAllowlistedApps() {
    std::string data;
    // Allowlist CTS by default.
    std::vector<std::string> allowlist = {kCtsAllowlist};

    if (!android::base::ReadFileToString(kAppAllowlistPath, &data)) {
        // Return default allowlist (no app can use extensions).
        LOG(INFO) << "Failed to read " << kAppAllowlistPath
                  << " ; No app allowlisted for vendor extensions use.";
        return allowlist;
    }

    std::istringstream streamData(data);
    std::string line;
    while (std::getline(streamData, line)) {
        // Do some basic validity check on entry, it's either
        // fs path or package name.
        if (StartsWith(line, "/") || line.find('.') != std::string::npos) {
            allowlist.push_back(line);
        } else {
            LOG(ERROR) << kAppAllowlistPath << " - Invalid entry: " << line;
        }
    }
    return allowlist;
}

// Since Android S we allow use of vendor extensions for all
// non-system applications without need to put the binary
// name on allowlist
static bool allowVendorExtensionsForAllNonSystemClients() {
#if defined(__BIONIC__)
    return android_get_device_api_level() >= __ANDROID_API_S__;
#else
    return true;
#endif  // __BIONIC__
}

#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

}  // namespace

TypeManager::TypeManager() {
    VLOG(MANAGER) << "TypeManager::TypeManager";
#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    mExtensionsAllowed = TypeManager::isExtensionsUseAllowed(
            AppInfoFetcher::get()->getAppInfo(), isNNAPIVendorExtensionsUseAllowedInProductImage(),
            getVendorExtensionAllowlistedApps());
#else
    mExtensionsAllowed = true;
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
    VLOG(MANAGER) << "NNAPI Vendor extensions enabled: " << mExtensionsAllowed;
    findAvailableExtensions();
}

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
bool TypeManager::isExtensionsUseAllowed(const AppInfoFetcher::AppInfo& appPackageInfo,
                                         bool useOnProductImageEnabled,
                                         const std::vector<std::string>& allowlist) {
    // Only selected partitions and user-installed apps (/data)
    // are allowed to use extensions.
    if (StartsWith(appPackageInfo.binaryPath, "/vendor/") ||
        StartsWith(appPackageInfo.binaryPath, "/odm/") ||
        StartsWith(appPackageInfo.binaryPath, "/data/") ||
        (StartsWith(appPackageInfo.binaryPath, "/product/") && useOnProductImageEnabled)) {
        if (allowVendorExtensionsForAllNonSystemClients()) {
            return true;
        }
#ifdef NN_DEBUGGABLE
        // Only on userdebug and eng builds.
        // When running tests with mma and adb push.
        if (StartsWith(appPackageInfo.binaryPath, "/data/nativetest") ||
            // When running tests with Atest.
            StartsWith(appPackageInfo.binaryPath, NN_TMP_DIR "/NeuralNetworksTest_")) {
            return true;
        }
#endif  // NN_DEBUGGABLE
        return std::find(allowlist.begin(), allowlist.end(), appPackageInfo.binaryPath) !=
               allowlist.end();
    } else if (appPackageInfo.binaryPath == "/system/bin/app_process64" ||
               appPackageInfo.binaryPath == "/system/bin/app_process32") {
        // App is (not system app) OR (vendor app) OR (product app AND product enabled)
        if (!appPackageInfo.appIsSystemApp || appPackageInfo.appIsOnVendorImage ||
            (appPackageInfo.appIsOnProductImage && useOnProductImageEnabled)) {
            if (allowVendorExtensionsForAllNonSystemClients()) {
                // No need for allowlist
                return true;
            } else {
                // Check if app is on allowlist.
                return std::find(allowlist.begin(), allowlist.end(),
                                 appPackageInfo.appPackageName) != allowlist.end();
            }
        }
    }
    return false;
}
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

void TypeManager::findAvailableExtensions() {
    for (const std::shared_ptr<Device>& device : mDeviceManager->getDrivers()) {
        for (const Extension& extension : device->getSupportedExtensions()) {
            registerExtension(extension, device->getName());
        }
    }
}

bool TypeManager::registerExtension(Extension extension, const std::string& deviceName) {
    if (mDisabledExtensions.find(extension.name) != mDisabledExtensions.end()) {
        LOG(ERROR) << "Extension " << extension.name << " is disabled";
        return false;
    }

    std::sort(extension.operandTypes.begin(), extension.operandTypes.end(),
              [](const Extension::OperandTypeInformation& a,
                 const Extension::OperandTypeInformation& b) {
                  return static_cast<uint16_t>(a.type) < static_cast<uint16_t>(b.type);
              });

    std::map<std::string, Extension>::iterator it;
    bool isNew;
    std::tie(it, isNew) = mExtensionNameToExtension.emplace(extension.name, extension);
    if (isNew) {
        VLOG(MANAGER) << "Registered extension " << extension.name;
        mExtensionNameToFirstDevice.emplace(extension.name, deviceName);
    } else if (!equal(extension, it->second)) {
        LOG(ERROR) << "Devices " << mExtensionNameToFirstDevice[extension.name] << " and "
                   << deviceName << " provide inconsistent information for extension "
                   << extension.name << ", which is therefore disabled";
        mExtensionNameToExtension.erase(it);
        mDisabledExtensions.insert(extension.name);
        return false;
    }
    return true;
}

bool TypeManager::getExtensionPrefix(const std::string& extensionName, uint16_t* prefix) {
    auto it = mExtensionNameToPrefix.find(extensionName);
    if (it != mExtensionNameToPrefix.end()) {
        *prefix = it->second;
    } else {
        NN_RET_CHECK_LE(mPrefixToExtension.size(), kMaxPrefix) << "Too many extensions in use";
        *prefix = mPrefixToExtension.size();
        mExtensionNameToPrefix[extensionName] = *prefix;
        mPrefixToExtension.push_back(&mExtensionNameToExtension[extensionName]);
    }
    return true;
}

std::vector<ExtensionNameAndPrefix> TypeManager::getExtensionNameAndPrefix(
        const std::vector<TokenValuePair>& metaData) {
    std::vector<ExtensionNameAndPrefix> extensionNameAndPrefix;
    std::set<uint16_t> prefixSet;
    for (auto p : metaData) {
        uint16_t prefix = static_cast<uint32_t>(p.token) >> kExtensionTypeBits;
        if (!prefixSet.insert(prefix).second) {
            continue;
        }
        const Extension* extension;
        CHECK(getExtensionInfo(prefix, &extension));
        extensionNameAndPrefix.push_back({
                .name = extension->name,
                .prefix = prefix,
        });
    }
    return extensionNameAndPrefix;
}

bool TypeManager::getExtensionType(const char* extensionName, uint16_t typeWithinExtension,
                                   int32_t* type) {
    uint16_t prefix;
    NN_RET_CHECK(getExtensionPrefix(extensionName, &prefix));
    *type = (prefix << kExtensionTypeBits) | typeWithinExtension;
    return true;
}

bool TypeManager::getExtensionInfo(uint16_t prefix, const Extension** extension) const {
    NN_RET_CHECK_NE(prefix, 0u) << "prefix=0 does not correspond to an extension";
    NN_RET_CHECK_LT(prefix, mPrefixToExtension.size()) << "Unknown extension prefix";
    *extension = mPrefixToExtension[prefix];
    return true;
}

bool TypeManager::getExtensionOperandTypeInfo(
        OperandType type, const Extension::OperandTypeInformation** info) const {
    uint32_t operandType = static_cast<uint32_t>(type);
    uint16_t prefix = operandType >> kExtensionTypeBits;
    uint16_t typeWithinExtension = operandType & ((1 << kExtensionTypeBits) - 1);
    const Extension* extension;
    NN_RET_CHECK(getExtensionInfo(prefix, &extension))
            << "Cannot find extension corresponding to prefix " << prefix;
    auto it = std::lower_bound(
            extension->operandTypes.begin(), extension->operandTypes.end(), typeWithinExtension,
            [](const Extension::OperandTypeInformation& info, uint32_t typeSought) {
                return static_cast<uint16_t>(info.type) < typeSought;
            });
    NN_RET_CHECK(it != extension->operandTypes.end() &&
                 static_cast<uint16_t>(it->type) == typeWithinExtension)
            << "Cannot find operand type " << typeWithinExtension << " in extension "
            << extension->name;
    *info = &*it;
    return true;
}

bool TypeManager::isTensorType(OperandType type) const {
    if (!isExtension(type)) {
        return !nonExtensionOperandTypeIsScalar(static_cast<int>(type));
    }
    const Extension::OperandTypeInformation* info;
    CHECK(getExtensionOperandTypeInfo(type, &info));
    return info->isTensor;
}

uint32_t TypeManager::getSizeOfData(OperandType type,
                                    const std::vector<uint32_t>& dimensions) const {
    if (!isExtension(type)) {
        return nonExtensionOperandSizeOfData(type, dimensions);
    }
    const Extension::OperandTypeInformation* info;
    CHECK(getExtensionOperandTypeInfo(type, &info));
    return info->isTensor ? sizeOfTensorData(info->byteSize, dimensions) : info->byteSize;
}

bool TypeManager::sizeOfDataOverflowsUInt32(OperandType type,
                                            const std::vector<uint32_t>& dimensions) const {
    if (!isExtension(type)) {
        return nonExtensionOperandSizeOfDataOverflowsUInt32(type, dimensions);
    }
    const Extension::OperandTypeInformation* info;
    CHECK(getExtensionOperandTypeInfo(type, &info));
    return info->isTensor ? sizeOfTensorDataOverflowsUInt32(info->byteSize, dimensions) : false;
}

}  // namespace nn
}  // namespace android
