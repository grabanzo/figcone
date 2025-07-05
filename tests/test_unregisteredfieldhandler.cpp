#include "assert_exception.h"
#include <figcone/config.h>
#include <figcone/configreader.h>
#include <figcone/unregisteredfieldhandler.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <optional>

namespace {

using namespace figcone;

struct Cfg : public Config {
    FIGCONE_PARAM(name, figcone::optional<std::string>);
    FIGCONE_PARAM(surname, std::string);
    FIGCONE_PARAM(age, int);
};

struct A : public figcone::Config {
    FIGCONE_PARAM(testInt, int);
};

struct B : public figcone::Config {
    FIGCONE_PARAM(testInt, int);
    FIGCONE_PARAM(testString, std::string)();
};

struct C : public figcone::Config {
    FIGCONE_PARAM(testInt, int)(9);
    FIGCONE_PARAM(testDouble, double)(9.0);
    FIGCONE_NODE(b, B);
};

struct SingleNodeSingleLevelCfg : public figcone::Config {
    FIGCONE_PARAM(foo, int);
    FIGCONE_PARAM(bar, std::string);
    FIGCONE_NODE(a, A);
};

struct MultiLevelCfg : public figcone::Config {
    FIGCONE_PARAM(foo, int);
    FIGCONE_PARAM(bar, std::string);
    FIGCONE_NODE(b, B);
    FIGCONE_NODE(c, C);
};

} //namespace


namespace figcone {
template<>
struct UnregisteredFieldHandler<B> {
    void operator()(figcone::FieldType, const std::string&, const figcone::StreamPosition&) {}
};
} //namespace figcone
namespace {

class TreeProvider : public figcone::IParser {
public:
    TreeProvider(std::unique_ptr<figcone::TreeNode> tree)
        : tree_{std::move(tree)}
    {
    }

    figcone::Tree parse(std::istream&) override
    {
        return std::move(tree_);
    }

    std::unique_ptr<figcone::TreeNode> tree_;
};

TEST(UnregisteredFieldHandler, IgnoreUnregisteredFields)
{
    ///foo = 5
    ///bar = test
    ///[c]
    ///  testInt = 11
    ///  testDouble = 12
    ///  [c.b]
    ///    testInt = 10
    ///    testString = 'Hello world'
    ///    unregisteredField = "foo"
    ///
    ///[b]
    ///  testInt = 9
    ///  unregisteredField = 42
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("foo", "5", {1, 1});
    tree->asItem().addParam("bar", "test", {2, 1});
    auto& cNode = tree->asItem().addNode("c", {3, 1});
    cNode.asItem().addParam("testInt", "11", {4, 3});
    cNode.asItem().addParam("testDouble", "12", {5, 3});
    auto& bNode = cNode.asItem().addNode("b", {6, 1});
    bNode.asItem().addParam("testInt", "10", {7, 3});
    bNode.asItem().addParam("testString", "Hello world", {8, 3});
    bNode.asItem().addParam("unregisteredField", "Foo", {9, 3});
    auto& bNode2 = tree->asItem().addNode("b", {11, 1});
    bNode2.asItem().addParam("testInt", "9", {12, 3});
    bNode2.asItem().addParam("unregisteredField", "42", {13, 3});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};

    auto cfg = cfgReader.read<MultiLevelCfg>("", parser);

    EXPECT_EQ(cfg.foo, 5);
    EXPECT_EQ(cfg.bar, "test");
    EXPECT_EQ(cfg.b.testInt, 9);
    EXPECT_EQ(cfg.b.testString, "");
    EXPECT_EQ(cfg.c.testInt, 11);
    EXPECT_EQ(cfg.c.testDouble, 12);
    EXPECT_EQ(cfg.c.b.testInt, 10);
    EXPECT_EQ(cfg.c.b.testString, "Hello world");
}

TEST(UnregisteredFieldHandler, UnregisteredFieldsWithoutHandlerNotIgnored)
{
    ///foo = 5
    ///bar = test
    ///unregisteredField = "foo"
    ///[c]
    ///  testInt = 11
    ///  testDouble = 12
    ///  [c.b]
    ///    testInt = 10
    ///    testString = 'Hello world'
    ///
    ///[b]
    ///  testInt = 9
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("foo", "5", {1, 1});
    tree->asItem().addParam("bar", "test", {2, 1});
    tree->asItem().addParam("unregisteredField", "foo", {3, 3});
    auto& cNode = tree->asItem().addNode("c", {4, 1});
    cNode.asItem().addParam("testInt", "11", {5, 3});
    cNode.asItem().addParam("testDouble", "12", {6, 3});
    auto& bNode = cNode.asItem().addNode("b", {7, 1});
    bNode.asItem().addParam("testInt", "10", {8, 3});
    bNode.asItem().addParam("testString", "Hello world", {9, 3});
    auto& bNode2 = tree->asItem().addNode("b", {12, 1});
    bNode2.asItem().addParam("testInt", "9", {13, 3});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<MultiLevelCfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(std::string{error.what()}, "[line:3, column:3] Unknown param 'unregisteredField'");
            });
}

} //namespace