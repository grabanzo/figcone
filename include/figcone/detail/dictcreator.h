#ifndef FIGCONE_DICTCREATOR_H
#define FIGCONE_DICTCREATOR_H

#include "configreaderaccess.h"
#include "dict.h"
#include "validator.h"
#include "external/eel/contract.h"
#include "external/eel/type_traits.h"
#include <figcone/nameformat.h>
#include <memory>

namespace figcone::detail {

template<typename TMap>
class DictCreator {
public:
    DictCreator(ConfigReaderPtr cfgReader, std::string dictName, TMap& dictMap, bool isOptional = false)
        : cfgReader_{cfgReader}
        , dictName_{(eel::precondition(!dictName.empty(), FIGCONE_EEL_LINE), std::move(dictName))}
        , dict_{std::make_unique<Dict<TMap>>(dictName_, dictMap)}
        , dictMap_{dictMap}
    {
        static_assert(
                eel::is_associative_container_v<eel::remove_optional_t<TMap>>,
                "Dictionary field must be an associative container or an associative container placed in "
                "std::optional");
        static_assert(
                std::is_same_v<typename eel::remove_optional_t<TMap>::key_type, std::string>,
                "Dictionary associative container's key type must be std::string");
        if (isOptional)
            dict_->markValueIsSet();
    }

    DictCreator& operator()(TMap defaultValue = {})
    {
        dict_->markValueIsSet();
        defaultValue_ = std::move(defaultValue);
        return *this;
    }

    void createDict()
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addNode(dictName_, std::move(dict_));
    }

    operator TMap()
    {
        createDict();
        return defaultValue_;
    }

    DictCreator& ensure(std::function<void(const eel::remove_optional_t<TMap>&)> validatingFunc)
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addValidator(
                    std::make_unique<Validator<TMap>>(*dict_, dictMap_, std::move(validatingFunc)));
        return *this;
    }

    template<typename TValidator, typename... TArgs>
    DictCreator& ensure(TArgs&&... args)
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addValidator(
                    std::make_unique<Validator<TMap>>(*dict_, dictMap_, TValidator{std::forward<TArgs>(args)...}));
        return *this;
    }

private:
    ConfigReaderPtr cfgReader_;
    std::string dictName_;
    std::unique_ptr<Dict<TMap>> dict_;
    TMap& dictMap_;
    TMap defaultValue_;
};

} //namespace figcone::detail

#endif //FIGCONE_DICTCREATOR_H