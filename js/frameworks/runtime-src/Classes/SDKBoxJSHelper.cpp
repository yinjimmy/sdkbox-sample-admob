#include "SDKBoxJSHelper.h"
#include <string>
#include <sstream>
#include "sdkbox/Sdkbox.h"

namespace sdkbox {

    JSListenerBase::JSListenerBase() {
        proFunc = nullptr;
    }

    void JSListenerBase::setJSDelegate(JSContext *cx, JS::HandleValue func) {
        if (nullptr != proFunc) {
            delete proFunc;
            proFunc = nullptr;
        }
        proFunc = new JS::PersistentRootedObject(cx, func.toObjectOrNull());
    }

    JSObject* JSListenerBase::getJSDelegate() {
        if (nullptr == proFunc) {
            return nullptr;
        }
        return proFunc->get();
    }

#if defined(MOZJS_MAJOR_VERSION)
#if MOZJS_MAJOR_VERSION >= 52
    JSOBJECT* JS_NEW_OBJECT( JSContext* cx ) {
        JS::RootedObject jsobj(cx);
        jsobj.set(JS_NewObject(cx, NULL));
        return jsobj;
    }
#elif MOZJS_MAJOR_VERSION >= 33
    JSOBJECT* JS_NEW_OBJECT( JSContext* cx ) {
        JS::RootedObject jsobj(cx);
        jsobj.set( JS_NewObject(cx, NULL, JS::NullPtr(), JS::NullPtr()) );
        return jsobj;
    }
#else
    JSOBJECT* JS_NEW_OBJECT( JSContext* cx ) {
        return JS_NewObject(cx, NULL, NULL, NULL);
    }

#endif

    template<typename T>
    bool JS_ARRAY_SET(JSContext* cx, JSOBJECT* array, uint32_t index, T pr) {
        return JS_SetElement(cx, JSPROPERTY_OBJECT(cx,array), index, pr);
    }

    bool JS_ARRAY_SET(JSContext* cx, JSOBJECT* array, uint32_t index, JSOBJECT* v) {
        JSPROPERTY_VALUE pr(cx);
        pr = JS::ObjectValue(*v);

#if MOZJS_MAJOR_VERSION <= 28
        return JS_SetElement(cx, JSPROPERTY_OBJECT(cx,array), index, &pr );
#else
        return JS_SetElement(cx, JSPROPERTY_OBJECT(cx,array), index, pr );
#endif
    }

#elif defined(JS_VERSION)

    template<typename T>
    bool JS_ARRAY_SET(JSContext* cx, JSOBJECT* array, uint32_t index, T pr) {
        return JS_SetElement(cx, array, index, &pr);
    }

    JSOBJECT* JS_NEW_OBJECT( JSContext* cx ) {
        return JS_NewObject(cx, NULL, NULL, NULL);
    }

#endif

    JSObject* make_array( JSContext* ctx, int size ) {
        // https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_reference/JS_NewArrayObject
#if MOZJS_MAJOR_VERSION >= 31
        return JS_NewArrayObject(ctx, size);
#else
        return JS_NewArrayObject(ctx, size, NULL);
#endif
    }

#if defined(JS_VERSION)
#define make_property(pr,CTX) JSPROPERTY_VALUE pr = jsval()
#else
#define make_property(pr,ctx)  JSPROPERTY_VALUE pr(ctx)
#endif


    JSOBJECT* JS_NEW_ARRAY( JSContext* cx, uint32_t size ) {
        return make_array(cx, size);
    }

    JSOBJECT* JS_NEW_ARRAY( JSContext* cx ) {
        return sdkbox::JS_NEW_ARRAY(cx, 0);
    }

    void addProperty( JSContext* cx, JSOBJECT* jsobj, const char* prop, const std::string& value ) {
        make_property(pr,cx);
        pr.set(JS::StringValue(JS_NewStringCopyZ(cx, value.c_str())));
        //pr = std_string_to_jsval(cx, value.c_str());
        JS_SET_PROPERTY(cx, jsobj, prop, pr);
    }
    void addProperty( JSContext* cx, JSOBJECT* jsobj, const char* prop, const char* value ) {
        make_property(pr,cx);
        pr.set(JS::StringValue(JS_NewStringCopyZ(cx, value)));
        //pr = std_string_to_jsval(cx, value);
        JS_SET_PROPERTY(cx, jsobj, prop, pr);
    }
    void addProperty( JSContext* cx, JSOBJECT* jsobj, const char* prop, bool value ) {
        make_property(pr,cx);
        pr.set(JS::BooleanValue(value));
        //pr = BOOLEAN_TO_JSVAL(value);
        JS_SET_PROPERTY(cx, jsobj, prop, pr);
    }
    void addProperty( JSContext* cx, JSOBJECT* jsobj, const char* prop, int value ) {
        make_property(pr,cx);
        pr.set(JS::Int32Value(value));
        //pr = INT_TO_JSVAL(value);
        JS_SET_PROPERTY(cx, jsobj, prop, pr);
    }
    void addProperty( JSContext* cx, JSOBJECT* jsobj, const char* prop, JSOBJECT* value ) {
        make_property(pr,cx);
        pr.set(JS::ObjectOrNullValue(value));
        //pr = OBJECT_TO_JSVAL(value);
        JS_SET_PROPERTY(cx, jsobj, prop, pr );
    }


    // Spidermonkey v186+
#if defined(MOZJS_MAJOR_VERSION)
#if MOZJS_MAJOR_VERSION >= 26

#if MOZJS_MAJOR_VERSION >= 52
    bool js_to_bool( JSContext *cx, JS::HandleValue vp, bool *ret ) {
        return ::jsval_to_bool(cx, vp, ret);
    }
#else
    bool js_to_bool( JSContext *cx, JS::HandleValue vp, bool *ret ) {
        bool ok = vp.isBoolean();
        if (ok) {
            *ret = vp.toBoolean();
        }
        return ok;
    }
#endif

    bool js_to_number(JSContext *cx, JS::HandleValue v, double *dp) {
        if (v.isNumber()) {
            *dp = v.toNumber();
            return true;
        } else {
            *dp = 0;
            return false;
        }
    }

    bool jsval_to_std_map_string_string(JSContext *cx, JS::HandleValue v, std::map<std::string,std::string> *ret)
    {
        cocos2d::ValueMap value;
        bool ok = jsval_to_ccvaluemap(cx, v, &value);
        if (!ok)
        {
            return ok;
        }
        else
        {
            for (cocos2d::ValueMap::iterator it = value.begin(); it != value.end(); it++)
            {
                ret->insert(std::make_pair(it->first, it->second.asString()));
            }
        }

        return ok;
    }

#if MOZJS_MAJOR_VERSION < 33
    void get_or_create_js_obj(JSContext* cx, JS::HandleObject obj, const std::string &name, JS::MutableHandleObject jsObj)
    {
        JS::RootedValue nsval(cx);
        JS_GetProperty(cx, obj, name.c_str(), &nsval);
        if (nsval == JSVAL_VOID) {
            jsObj.set(JS_NewObject(cx, NULL, NULL, NULL));
            nsval = OBJECT_TO_JSVAL(jsObj);
            JS_SetProperty(cx, obj, name.c_str(), nsval);
        } else {
            jsObj.set(nsval.toObjectOrNull());
        }
    }
#endif

    void getJsObjOrCreat(JSContext* cx, JS::HandleObject jsObj, const char* name, JS::MutableHandleObject retObj) {
        JS::RootedObject parent(cx);
        JS::RootedObject tempObj(cx);
        bool first = true;

        std::stringstream ss(name);
        std::string sub;
        const char* subChar;
        while(getline(ss, sub, '.')) {
            if(sub.empty())continue;

            subChar = sub.c_str();
            if (first) {
                get_or_create_js_obj(cx, jsObj, subChar, &tempObj);
                first = false;
            } else {
                parent = tempObj;
                get_or_create_js_obj(cx, parent, subChar, &tempObj);
            }
        }

        retObj.set(tempObj.get());
    }

#else
    JSBool js_to_number(JSContext *cx, jsval v, double *dp)
    {
        return JS::ToNumber( cx, v, dp) && !isnan(*dp);
    }

    bool jsval_to_std_map_string_string(JSContext *cx, JS::HandleValue v, std::map<std::string,std::string> *ret)
    {
        cocos2d::ValueMap value;
        bool ok = jsval_to_ccvaluemap(cx, v, &value);
        if (!ok)
        {
            return ok;
        }
        else
        {
            for (cocos2d::ValueMap::iterator it = value.begin(); it != value.end(); it++)
            {
                ret->insert(std::make_pair(it->first, it->second.asString()));
            }
        }

        return ok;
    }

    jsval getJsObjOrCreat(JSContext* cx, JSObject* jsObj, const char* name, JSObject** retObj) {
        JSObject* parent = NULL;
        JSObject* tempObj = jsObj;
        jsval tempVal;

        std::stringstream ss(name);
        std::string sub;
        const char* subChar;

        while(getline(ss, sub, '.')) {
            if(sub.empty())continue;

            subChar = sub.c_str();
            parent = tempObj;
            JS_GetProperty(cx, parent, subChar, &tempVal);
            if (tempVal == JSVAL_VOID) {
                tempObj = JS_NewObject(cx, NULL, NULL, NULL);
                tempVal = OBJECT_TO_JSVAL(tempObj);
                JS_SetProperty(cx, parent, subChar, &tempVal);
            } else {
                JS_ValueToObject(cx, tempVal, &tempObj);
            }
        }

        *retObj = tempObj;
        return tempVal;
    }

#endif

    // Spidermonkey v186 and earlier
#else
    JSBool js_to_number(JSContext *cx, jsval v, double *dp)
    {
        return JS_ValueToNumber(cx, v, dp);
    }

    JSBool jsval_to_std_map_string_string(JSContext *cx, jsval v, std::map<std::string,std::string> *ret)
    {
        cocos2d::CCDictionary* value;
        bool ok = jsval_to_ccdictionary(cx, v, &value);
        if (!ok)
        {
            return ok;
        }
        else
        {
            CCDictElement* pElement;
            CCDICT_FOREACH(value, pElement)
            {
                const char*key = pElement->getStrKey();
                CCString *v = (CCString*) pElement->getObject();
                ret->insert(std::make_pair(key, v->getCString()));
            }
        }

        return ok;
    }

    jsval getJsObjOrCreat(JSContext* cx, JSObject* jsObj, const char* name, JSObject** retObj) {
        JSObject* parent = NULL;
        JSObject* tempObj = jsObj;
        jsval tempVal;

        std::stringstream ss(name);
        std::string sub;
        const char* subChar;

        while(getline(ss, sub, '.')) {
            if(sub.empty())continue;

            subChar = sub.c_str();
            parent = tempObj;
            JS_GetProperty(cx, parent, subChar, &tempVal);
            if (tempVal == JSVAL_VOID) {
                tempObj = JS_NewObject(cx, NULL, NULL, NULL);
                tempVal = OBJECT_TO_JSVAL(tempObj);
                JS_SetProperty(cx, parent, subChar, &tempVal);
            } else {
                JS_ValueToObject(cx, tempVal, &tempObj);
            }
        }

        *retObj = tempObj;
        return tempVal;
    }

    JSBool jsval_to_std_vector_string(JSContext* cx, jsval v, std::vector<std::string>* ret) {
        JSObject *jsobj;
        JSBool ok = JS_ValueToObject( cx, v, &jsobj );
        JSB_PRECONDITION2( ok, cx, JS_FALSE, "Error converting value to object");
        JSB_PRECONDITION2( jsobj && JS_IsArrayObject( cx, jsobj),  cx, JS_FALSE, "Object must be an array");

        uint32_t len = 0;
        JS_GetArrayLength(cx, jsobj, &len);

        for (uint32_t i=0; i < len; i++) {
            jsval elt;
            if (JS_GetElement(cx, jsobj, i, &elt)) {

                if (JSVAL_IS_STRING(elt))
                {
                    JSStringWrapper str(JSVAL_TO_STRING(elt));
                    ret->push_back(str.get());
                }
            }
        }

        return JS_TRUE;
    }

#endif

#if MOZJS_MAJOR_VERSION >= 52
    bool c_string_to_jsval(JSContext* cx, const char* v, JS::MutableHandleValue ret, size_t length) {
        return ::c_string_to_jsval(cx, v, ret, length);
    }
#else
    bool c_string_to_jsval(JSContext* cx, const char* v, JS::MutableHandleValue ret, size_t length) {
        if (v == NULL) {
            return false;
        }
        if (length == -1) {
            length = strlen(v);
        }

        if (0 == length) {
            auto emptyStr = JS_NewStringCopyZ(cx, "");
            ret.set(JS::StringValue(emptyStr));
            return true;
        }

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
        // NOTE: Visual Studio 2013 (Platform Toolset v120) is not fully C++11 compatible.
        // It also doesn't provide support for char16_t and std::u16string.
        // For more information, please see this article
        // https://blogs.msdn.microsoft.com/vcblog/2014/11/17/c111417-features-in-vs-2015-preview/
        int utf16_size = 0;
        const jschar* strUTF16 = (jschar*)cc_utf8_to_utf16(v, (int)length, &utf16_size);

        if (strUTF16 && utf16_size > 0) {
            JSString* str = JS_NewUCStringCopyN(cx, strUTF16, (size_t)utf16_size);
            if (str) {
                ret.set(JS::StringValue(str));
            }
            delete[] strUTF16;
        }
#else
        std::u16string strUTF16;
        bool ok = cocos2d::StringUtils::UTF8ToUTF16(std::string(v, length), strUTF16);

        if (ok && !strUTF16.empty()) {
            JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<const char16_t*>(strUTF16.data()), strUTF16.size());
            if (str) {
                ret.set(JS::StringValue(str));
            }
        }
#endif

        return true;
    }
#endif

    void std_map_string_int_to_jsval(JSContext* cx, const std::map<std::string, int>& v, JS::MutableHandleValue retVal) {
        JS::RootedObject proto(cx);
        JS::RootedObject parent(cx);
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 52
        JS::RootedObject jsRet(cx, JS_NewObject(cx, NULL));
#elif defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
        JS::RootedObject jsRet(cx, JS_NewObject(cx, NULL, proto, parent));
#else
        JSObject *jsRet = JS_NewObject(cx, NULL, NULL, NULL);
#endif

        for (auto iter = v.begin(); iter != v.end(); ++iter) {
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
            JS::RootedValue element(cx);
#else
            jsval element;
#endif

            std::string key = iter->first;
            int val = iter->second;

            element = JS::Int32Value(val);

            if (!key.empty()) {
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
                JS_SetProperty(cx, jsRet, key.c_str(), element);
#else
                JS_SetProperty(cx, jsRet, key.c_str(), &element);
#endif
            }
        }

        retVal.set(JS::ObjectOrNullValue(jsRet.get()));
    }

    void std_map_string_string_to_jsval(JSContext* cx, const std::map<std::string, std::string>& v, JS::MutableHandleValue retVal) {
        JS::RootedObject proto(cx);
        JS::RootedObject parent(cx);
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 52
        JS::RootedObject jsRet(cx, JS_NewObject(cx, NULL));
#elif defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
        JS::RootedObject jsRet(cx, JS_NewObject(cx, NULL, proto, parent));
#else
        JSObject *jsRet = JS_NewObject(cx, NULL, NULL, NULL);
#endif

        for (auto iter = v.begin(); iter != v.end(); ++iter) {
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
            JS::RootedValue element(cx);
#else
            jsval element;
#endif

            std::string key = iter->first;
            std::string val = iter->second;

            JSString* jsstr = JS_NewStringCopyZ(cx, val.c_str());
            element = JS::StringValue(jsstr);

            if (!key.empty()) {
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
                JS_SetProperty(cx, jsRet, key.c_str(), element);
#else
                JS_SetProperty(cx, jsRet, key.c_str(), &element);
#endif
            }
        }

        retVal.set(JS::ObjectOrNullValue(jsRet.get()));
    }

    bool std_vector_string_to_jsval(JSContext* cx, const std::vector<std::string>& arr, JS::MutableHandleValue retVal) {
#if defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 52
        return ::std_vector_string_to_jsval(cx, arr, retVal);
#elif defined(MOZJS_MAJOR_VERSION) and MOZJS_MAJOR_VERSION >= 26
        JS::Value val = ::std_vector_string_to_jsval(cx, arr);
        retVal.set(val);
#else
        //support v2?
        JSObject *jsretArr = JS_NewArrayObject(cx, 0, NULL);

        int i = 0;
        for(std::vector<std::string>::const_iterator iter = arr.begin(); iter != arr.end(); ++iter, ++i) {
            jsval arrElement = c_string_to_jsval(cx, iter->c_str());
            if(!JS_SetElement(cx, jsretArr, i, &arrElement)) {
                break;
            }
        }
        return OBJECT_TO_JSVAL(jsretArr);
#endif
    }

}
