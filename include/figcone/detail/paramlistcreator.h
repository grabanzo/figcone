#ifndef FIGCONE_PARAMLISTCREATOR_H
#define FIGCONE_PARAMLISTCREATOR_H

#include "configreaderaccess.h"
#include "inode.h"
#include "paramlist.h"
#include "utils.h"
#include "validator.h"
#include "external/eel/contract.h"
#include "external/eel/type_traits.h"
#include <vector>

namespace figcone::detail {

template<typename TParamList>
class ParamListCreator {
    static_assert(
            eel::is_dynamic_sequence_container_v<eel::remove_optional_t<TParamList>>,
            "Param list field must be a sequence container or a sequence container placed in std::optional");

public:
    ParamListCreator(
            ConfigReaderPtr cfgReader,
            std::string paramListName,
            TParamList& paramListValue,
            bool isOptional = false)
        : cfgReader_{cfgReader}
        , paramListName_{(eel::precondition(!paramListName.empty(), FIGCONE_EEL_LINE), std::move(paramListName))}
        , paramListValue_{paramListValue}
        , paramList_{std::make_unique<ParamList<TParamList>>(paramListName_, paramListValue)}
    {
        if (isOptional)
            paramList_->markValueIsSet();
    }

    ParamListCreator<TParamList>& operator()(TParamList defaultValue = {})
    {
        defaultValue_ = std::move(defaultValue);
        paramList_->markValueIsSet();
        return *this;
    }

    ParamListCreator<TParamList>& ensure(std::function<void(const eel::remove_optional_t<TParamList>&)> validatingFunc)
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addValidator(
                    std::make_unique<Validator<TParamList>>(*paramList_, paramListValue_, std::move(validatingFunc)));
        return *this;
    }

    template<typename TValidator, typename... TArgs>
    ParamListCreator<TParamList>& ensure(TArgs&&... args)
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addValidator(std::make_unique<Validator<TParamList>>(
                    *paramList_,
                    paramListValue_,
                    TValidator{std::forward<TArgs>(args)...}));
        return *this;
    }

    void createParamList()
    {
        if (cfgReader_)
            ConfigReaderAccess{cfgReader_}.addParam(paramListName_, std::move(paramList_));
    }

    operator TParamList()
    {
        createParamList();
        return defaultValue_;
    }

private:
    ConfigReaderPtr cfgReader_;
    std::string paramListName_;
    TParamList& paramListValue_;
    std::unique_ptr<ParamList<TParamList>> paramList_;
    TParamList defaultValue_;
};

} //namespace figcone::detail

#endif //FIGCONE_PARAMLISTCREATOR_H