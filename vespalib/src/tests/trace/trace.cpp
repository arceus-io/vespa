// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/log/log.h>
LOG_SETUP("trace_test");
#include <vespa/vespalib/testkit/testapp.h>
#include <vespa/vespalib/trace/trace.h>
#include <vespa/vespalib/trace/tracevisitor.h>

using namespace vespalib;

class Test : public vespalib::TestApp {
private:
    void testEncodeDecode();
    void testReservedChars();
    void testConstruct();
    void testAdd();
    void testSort();
    void testStrict();
    void testTraceLevel();
    void testCompact();
    void testNormalize();
    void testTraceDump();
    void testVisiting();
    void testTimestamp();

public:
    int Main() override;
};

TEST_APPHOOK(Test);

int
Test::Main()
{
    TEST_INIT("trace_test");
    testEncodeDecode();
    testReservedChars();
    testAdd();
    testConstruct();
    testSort();
    testStrict();
    testTraceLevel();
    testCompact();
    testNormalize();
    testTraceDump();
    testVisiting();
    testTimestamp();
    TEST_DONE();
}

void
Test::testEncodeDecode()
{
    EXPECT_EQUAL("()", TraceNode::decode("").encode());
    EXPECT_EQUAL("()", TraceNode::decode("[xyz").encode());
    EXPECT_EQUAL("([xyz][])", TraceNode::decode("[xyz][]").encode());
    EXPECT_EQUAL("[xyz]", TraceNode::decode("[xyz]").encode());
    EXPECT_EQUAL("()", TraceNode::decode("{()").encode());
    EXPECT_EQUAL("({()}{})", TraceNode::decode("{()}{}").encode());
    EXPECT_EQUAL("{()}", TraceNode::decode("{()}").encode());
    EXPECT_EQUAL("()", TraceNode::decode("({}").encode());
    EXPECT_EQUAL("(({})())", TraceNode::decode("({})()").encode());
    EXPECT_EQUAL("([])", TraceNode::decode("([])").encode());

    EXPECT_TRUE(TraceNode::decode("").isEmpty());
    EXPECT_TRUE(!TraceNode::decode("([note])").isEmpty());

    string str =
        "([[17/Jun/2009:09:02:30 +0200\\] Message (type 1) received at 'dst' for session 'session'.]"
        "[[17/Jun/2009:09:02:30 +0200\\] [APP_TRANSIENT_ERROR @ localhost\\]: err1]"
        "[[17/Jun/2009:09:02:30 +0200\\] Sending reply (version 4.2) from 'dst'.])";
    fprintf(stderr, "%s\n", TraceNode::decode(str).toString().c_str());
    EXPECT_EQUAL(str, TraceNode::decode(str).encode());

    str = "([Note 0][Note 1]{[Note 2]}{([Note 3])({[Note 4]})})";
    TraceNode t = TraceNode::decode(str);
    EXPECT_EQUAL(str, t.encode());

    EXPECT_TRUE(t.isRoot());
    EXPECT_TRUE(t.isStrict());
    EXPECT_TRUE(!t.isLeaf());
    EXPECT_EQUAL(4u, t.getNumChildren());

    {
        TraceNode c = t.getChild(0);
        EXPECT_TRUE(c.isLeaf());
        EXPECT_EQUAL("Note 0", c.getNote());
    }
    {
        TraceNode c = t.getChild(1);
        EXPECT_TRUE(c.isLeaf());
        EXPECT_EQUAL("Note 1", c.getNote());
    }
    {
        TraceNode c = t.getChild(2);
        EXPECT_TRUE(!c.isLeaf());
        EXPECT_TRUE(!c.isStrict());
        EXPECT_EQUAL(1u, c.getNumChildren());
        {
            TraceNode d = c.getChild(0);
            EXPECT_TRUE(d.isLeaf());
            EXPECT_EQUAL("Note 2", d.getNote());
        }
    }
    {
        TraceNode c = t.getChild(3);
        EXPECT_TRUE(!c.isStrict());
        EXPECT_EQUAL(2u, c.getNumChildren());
        {
            TraceNode d = c.getChild(0);
            EXPECT_TRUE(d.isStrict());
            EXPECT_TRUE(!d.isLeaf());
            EXPECT_EQUAL(1u, d.getNumChildren());
            {
                TraceNode e = d.getChild(0);
                EXPECT_TRUE(e.isLeaf());
                EXPECT_EQUAL("Note 3", e.getNote());
            }
        }
        {
            TraceNode d = c.getChild(1);
            EXPECT_TRUE(d.isStrict());
            EXPECT_EQUAL(1u, d.getNumChildren());
            {
                TraceNode e = d.getChild(0);
                EXPECT_TRUE(!e.isStrict());
                EXPECT_EQUAL(1u, e.getNumChildren());
                {
                    TraceNode f = e.getChild(0);
                    EXPECT_TRUE(f.isLeaf());
                    EXPECT_EQUAL("Note 4", f.getNote());
                }
            }
        }
    }
}

void
Test::testReservedChars()
{
    TraceNode t;
    t.addChild("abc(){}[]\\xyz");
    EXPECT_EQUAL("abc(){}[]\\xyz", t.getChild(0).getNote());
    EXPECT_EQUAL("([abc(){}[\\]\\\\xyz])", t.encode());
    {
        // test swap/clear/empty here
        TraceNode t2;
        EXPECT_TRUE(t2.isEmpty());
        t2.swap(t);
        EXPECT_TRUE(!t2.isEmpty());
        EXPECT_EQUAL("abc(){}[]\\xyz", t2.getChild(0).getNote());
        EXPECT_EQUAL("([abc(){}[\\]\\\\xyz])", t2.encode());
        t2.clear();
        EXPECT_TRUE(t2.isEmpty());
    }
}

void
Test::testAdd()
{
    TraceNode t1 = TraceNode::decode("([x])");
    TraceNode t2 = TraceNode::decode("([y])");
    TraceNode t3 = TraceNode::decode("([z])");

    t1.addChild(t2);
    EXPECT_EQUAL("([x]([y]))", t1.encode());
    EXPECT_TRUE(t1.getChild(1).isStrict());
    t1.addChild("txt");
    EXPECT_TRUE(t1.getChild(2).isLeaf());
    EXPECT_EQUAL("([x]([y])[txt])", t1.encode());
    t3.addChild(t1);
    EXPECT_EQUAL("([z]([x]([y])[txt]))", t3.encode());

    // crazy but possible (everything is by value)
    t2.addChild(t2).addChild(t2);
    EXPECT_EQUAL("([y]([y])([y]([y])))", t2.encode());
}

void
Test::testStrict()
{
    EXPECT_EQUAL("{}", TraceNode::decode("()").setStrict(false).encode());
    EXPECT_EQUAL("{[x]}", TraceNode::decode("([x])").setStrict(false).encode());
    EXPECT_EQUAL("{[x][y]}", TraceNode::decode("([x][y])").setStrict(false).encode());
}

void
Test::testTraceLevel()
{
    Trace t;
    t.setLevel(4);
    EXPECT_EQUAL(4u, t.getLevel());
    t.trace(9, "no");
    EXPECT_EQUAL(0u, t.getRoot().getNumChildren());
    t.trace(8, "no");
    EXPECT_EQUAL(0u, t.getRoot().getNumChildren());
    t.trace(7, "no");
    EXPECT_EQUAL(0u, t.getRoot().getNumChildren());
    t.trace(6, "no");
    EXPECT_EQUAL(0u, t.getRoot().getNumChildren());
    t.trace(5, "no");
    EXPECT_EQUAL(0u, t.getRoot().getNumChildren());
    t.trace(4, "yes");
    EXPECT_EQUAL(1u, t.getRoot().getNumChildren());
    t.trace(3, "yes");
    EXPECT_EQUAL(2u, t.getRoot().getNumChildren());
    t.trace(2, "yes");
    EXPECT_EQUAL(3u, t.getRoot().getNumChildren());
    t.trace(1, "yes");
    EXPECT_EQUAL(4u, t.getRoot().getNumChildren());
    t.trace(0, "yes");
    EXPECT_EQUAL(5u, t.getRoot().getNumChildren());
}

void
Test::testCompact()
{
    EXPECT_EQUAL("()", TraceNode::decode("()").compact().encode());
    EXPECT_EQUAL("()", TraceNode::decode("(())").compact().encode());
    EXPECT_EQUAL("()", TraceNode::decode("(()())").compact().encode());
    EXPECT_EQUAL("()", TraceNode::decode("({})").compact().encode());
    EXPECT_EQUAL("()", TraceNode::decode("({}{})").compact().encode());
    EXPECT_EQUAL("()", TraceNode::decode("({{}{}})").compact().encode());

    EXPECT_EQUAL("([x])", TraceNode::decode("([x])").compact().encode());
    EXPECT_EQUAL("([x])", TraceNode::decode("(([x]))").compact().encode());
    EXPECT_EQUAL("([x][y])", TraceNode::decode("(([x])([y]))").compact().encode());
    EXPECT_EQUAL("([x])", TraceNode::decode("({[x]})").compact().encode());
    EXPECT_EQUAL("([x][y])", TraceNode::decode("({[x]}{[y]})").compact().encode());
    EXPECT_EQUAL("({[x][y]})", TraceNode::decode("({{[x]}{[y]}})").compact().encode());

    EXPECT_EQUAL("([a][b][c][d])", TraceNode::decode("(([a][b])([c][d]))").compact().encode());
    EXPECT_EQUAL("({[a][b]}{[c][d]})", TraceNode::decode("({[a][b]}{[c][d]})").compact().encode());
    EXPECT_EQUAL("({[a][b][c][d]})", TraceNode::decode("({{[a][b]}{[c][d]}})").compact().encode());
    EXPECT_EQUAL("({([a][b])([c][d])})", TraceNode::decode("({([a][b])([c][d])})").compact().encode());

    EXPECT_EQUAL("({{}{(({()}({}){()(){}}){})}})", TraceNode::decode("({{}{(({()}({}){()(){}}){})}})").encode());
    EXPECT_EQUAL("()", TraceNode::decode("({{}{(({()}({}){()(){}}){})}})").compact().encode());
    EXPECT_EQUAL("([x])", TraceNode::decode("({{}{([x]({()}({}){()(){}}){})}})").compact().encode());
    EXPECT_EQUAL("([x])", TraceNode::decode("({{}{(({()}({[x]}){()(){}}){})}})").compact().encode());
    EXPECT_EQUAL("([x])", TraceNode::decode("({{}{(({()}({}){()(){}})[x]{})}})").compact().encode());

    EXPECT_EQUAL("({[a][b][c][d][e][f]})", TraceNode::decode("({({[a][b]})({[c][d]})({[e][f]})})").compact().encode());
}

void
Test::testSort()
{
    EXPECT_EQUAL("([b][a][c])", TraceNode::decode("([b][a][c])").sort().encode());
    EXPECT_EQUAL("({[a][b][c]})", TraceNode::decode("({[b][a][c]})").sort().encode());
    EXPECT_EQUAL("(([c][a])([b]))", TraceNode::decode("(([c][a])([b]))").sort().encode());
    EXPECT_EQUAL("({[b]([c][a])})", TraceNode::decode("({([c][a])[b]})").sort().encode());
    EXPECT_EQUAL("({[a][c]}[b])", TraceNode::decode("({[c][a]}[b])").sort().encode());
    EXPECT_EQUAL("({([b]){[a][c]}})", TraceNode::decode("({{[c][a]}([b])})").sort().encode());
}

void
Test::testNormalize()
{
    TraceNode t1 = TraceNode::decode("({([a][b]{[x][y]([p][q])})([c][d])([e][f])})");
    TraceNode t2 = TraceNode::decode("({([a][b]{[y][x]([p][q])})([c][d])([e][f])})");
    TraceNode t3 = TraceNode::decode("({([a][b]{[y]([p][q])[x]})([c][d])([e][f])})");
    TraceNode t4 = TraceNode::decode("({([e][f])([a][b]{[y]([p][q])[x]})([c][d])})");
    TraceNode t5 = TraceNode::decode("({([e][f])([c][d])([a][b]{([p][q])[y][x]})})");

    TraceNode tx = TraceNode::decode("({([b][a]{[x][y]([p][q])})([c][d])([e][f])})");
    TraceNode ty = TraceNode::decode("({([a][b]{[x][y]([p][q])})([d][c])([e][f])})");
    TraceNode tz = TraceNode::decode("({([a][b]{[x][y]([q][p])})([c][d])([e][f])})");

    EXPECT_EQUAL("({([a][b]{[x][y]([p][q])})([c][d])([e][f])})", t1.compact().encode());

    EXPECT_TRUE(t1.compact().encode() != t2.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != t3.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != t4.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != t5.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != tx.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != ty.compact().encode());
    EXPECT_TRUE(t1.compact().encode() != tz.compact().encode());

    fprintf(stderr, "1: %s\n", + t1.normalize().encode().c_str());
    fprintf(stderr, "2: %s\n", + t2.normalize().encode().c_str());
    fprintf(stderr, "3: %s\n", + t3.normalize().encode().c_str());
    fprintf(stderr, "4: %s\n", + t4.normalize().encode().c_str());
    fprintf(stderr, "5: %s\n", + t5.normalize().encode().c_str());
    fprintf(stderr, "x: %s\n", + tx.normalize().encode().c_str());
    fprintf(stderr, "y: %s\n", + ty.normalize().encode().c_str());
    fprintf(stderr, "z: %s\n", + tz.normalize().encode().c_str());
    EXPECT_TRUE(t1.normalize().encode() == t2.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() == t3.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() == t4.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() == t5.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() != tx.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() != ty.normalize().encode());
    EXPECT_TRUE(t1.normalize().encode() != tz.normalize().encode());

    EXPECT_EQUAL("({([c][d])([e][f])([a][b]{[x][y]([p][q])})})", t1.normalize().encode());
}

void
Test::testTraceDump()
{
    {
        Trace big;
        TraceNode b1;
        TraceNode b2;
        for (int i = 0; i < 100; ++i) {
            b2.addChild("test");
        }
        for (int i = 0; i < 10; ++i) {
            b1.addChild(b2);
        }
        for (int i = 0; i < 10; ++i) {
            big.getRoot().addChild(b1);
        }
        string normal = big.toString();
        string full = big.getRoot().toString();
        EXPECT_GREATER(normal.size(), 30000u);
        EXPECT_LESS(normal.size(), 32000u);
        EXPECT_GREATER(full.size(), 50000u);
        EXPECT_EQUAL(0, strncmp(normal.c_str(), full.c_str(), 30000));
    }
    {
        TraceNode s1;
        TraceNode s2;
        s2.addChild("test");
        s2.addChild("test");
        s1.addChild(s2);
        s1.addChild(s2);
        EXPECT_EQUAL(vespalib::string("...\n"), s1.toString(0));
        EXPECT_EQUAL(vespalib::string("<trace>\n...\n"), s1.toString(1));
        EXPECT_EQUAL(vespalib::string("<trace>\n"      // 8    8
                                      "    <trace>\n"  // 12  20
                                      "        test\n" // 13  33
                                      "...\n"), s1.toString(33));
        EXPECT_EQUAL(vespalib::string("<trace>\n"      // 8   8
                                      "    test\n"     // 9  17
                                      "    test\n"     // 9  26
                                      "...\n"), s2.toString(26));
        EXPECT_EQUAL(vespalib::string("<trace>\n"      // 8   8
                                      "    test\n"     // 9  17
                                      "    test\n"     // 9  26
                                      "</trace>\n"), s2.toString(27));
        EXPECT_EQUAL(s2.toString(27), s2.toString());
    }
}

struct EncoderVisitor : public TraceVisitor
{
    vespalib::string str;
    void entering(const TraceNode & traceNode) override {
        (void) traceNode;
        str += "(";
    }
    void visit(const TraceNode & traceNode) override {
        if (traceNode.hasNote()) {
            str += "[";
            str += traceNode.getNote();
            str += "]";
        }
    }
    void leaving(const TraceNode & traceNode) override {
        (void) traceNode;
        str += ")";
    }
};

void
Test::testVisiting()
{
    TraceNode b1;
    TraceNode b2;
    for (int i = 0; i < 100; ++i) {
        std::stringstream ss;
        ss << i;
        TraceNode b3;
        b3.addChild(ss.str());
        b2.addChild(b3);
    }
    for (int i = 0; i < 10; ++i) {
        b1.addChild(b2);
    }
    EncoderVisitor encoder;
    b1.accept(encoder);
    EXPECT_EQUAL(encoder.str, b1.encode());
}

void
Test::testTimestamp()
{
    TraceNode root;
    root.addChild("foo", 1234);
    root.addChild("bar");
    EXPECT_EQUAL(root.getTimestamp(), 0);
    EXPECT_EQUAL(root.getChild(0).getTimestamp(), 1234);
    EXPECT_EQUAL(root.getChild(1).getTimestamp(), 0);
}

void
Test::testConstruct()
{
    TraceNode leaf1("foo", 123);
    EXPECT_TRUE(leaf1.hasNote());
    EXPECT_EQUAL("foo", leaf1.getNote());
    EXPECT_EQUAL(123, leaf1.getTimestamp());

    TraceNode leaf2(124);
    EXPECT_FALSE(leaf2.hasNote());
    EXPECT_EQUAL("", leaf2.getNote());
    EXPECT_EQUAL(124, leaf2.getTimestamp());
}
