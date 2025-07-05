#ifndef FIGCONE_DICT_H
#define FIGCONE_DICT_H

#include "inode.h"
#include "param.h"
#include "utils.h"
#include "external/eel/type_traits.h"
#include <figcone_tree/tree.h>
#include <map>
#include <string>
#include <type_traits>

namespace figcone::detail {

template<typename TMap>
class Dict : public INode {
public:
    explicit Dict(std::string name, TMap& dictMap)
        : name_{std::move(name)}
        , dictMap_{dictMap}
    {
        static_assert(
                eel::is_associative_container_v<eel::remove_optional_t<TMap>>,
                "Dictionary field must be an associative container or an associative container placed in "
                "std::optional");
        static_assert(
                std::is_same_v<typename eel::remove_optional_t<TMap>::key_type, std::string>,
                "Dictionary associative container's key type must be std::string");
    }

    void markValueIsSet()
    {
        hasValue_ = true;
    }

private:
    void load(const TreeNode& node) override
    {
        hasValue_ = true;
        position_ = node.position();
        dictMap_ = TMap{};
        if (!node.isItem())
            throw ConfigError{"Dictionary '" + name_ + "': config node can't be a list.", node.position()};
        if constexpr (eel::is_optional_v<TMap>)
            dictMap_.emplace();

        for (const auto& paramName : node.asItem().paramNames()) {
            const auto& paramValue = node.asItem().param(paramName);
            using Param = typename eel::remove_optional_t<TMap>::mapped_type;
            const auto paramNameStr = paramName;
            const auto paramValueStr = paramValue;
            auto paramReadResult = convertFromString<Param>(paramValueStr.value());
            auto readResultVisitor = eel::overloaded{
                    [&](const Param& param)
                    {
                        maybeOptValue(dictMap_).emplace(paramNameStr, param);
                    },
                    [&](const StringConversionError& error)
                    {
                        throw ConfigError{
                                "Couldn't set dict element'" + name_ + "' value from '" + paramValueStr.value() + "'" +
                                        (!error.message.empty() ? ": " + error.message : ""),
                                position_};
                    }};
            std::visit(readResultVisitor, paramReadResult);
        }
    }

    bool hasValue() const override
    {
        if constexpr (eel::is_optional_v<TMap>)
            return true;
        else
            return hasValue_;
    }

    StreamPosition position() override
    {
        return position_;
    }

    std::string description() override
    {
        return "Dictionary '" + name_ + "'";
    }

private:
    std::string name_;
    TMap& dictMap_;
    bool hasValue_ = false;
    StreamPosition position_;
};

} //namespace figcone::detail

#endif //FIGCONE_DICT_H