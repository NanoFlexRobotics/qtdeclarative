/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>

#include "testmodel.h"

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

#define LOAD_TREEVIEW(fileName) \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    QVERIFY(QTest::qWaitForWindowActive(view)); \
    auto treeView = view->rootObject()->property("treeView").value<QQuickTreeView *>(); \
    QVERIFY(treeView); \
    auto treeViewPrivate = QQuickTreeViewPrivate::get(treeView); \
    Q_UNUSED(treeViewPrivate) \
    auto model = treeView->model().value<TestModel *>(); \
    Q_UNUSED(model)

#define WAIT_UNTIL_POLISHED_ARG(item) \
    QVERIFY(QQuickTest::qIsPolishScheduled(item)); \
    QVERIFY(QQuickTest::qWaitForItemPolished(item))
#define WAIT_UNTIL_POLISHED WAIT_UNTIL_POLISHED_ARG(treeView)

// ########################################################

class tst_qquicktreeview : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktreeview();

private:
    QQuickView *view = nullptr;

private slots:
    void initTestCase() override;
    void showTreeView();
    void invalidArguments();
    void expandAndCollapseRoot();
    void toggleExpanded();
    void expandAndCollapseChildren();
    void expandChildPendingToBeVisible();
    void expandRecursivelyRoot_data();
    void expandRecursivelyRoot();
    void expandRecursivelyChild_data();
    void expandRecursivelyChild();
    void expandRecursivelyWholeTree();
    void collapseRecursivelyRoot();
    void collapseRecursivelyChild();
    void collapseRecursivelyWholeTree();
    void expandToIndex();
    void requiredPropertiesRoot();
    void requiredPropertiesChildren();
    void emptyModel();
    void updatedModifiedModel();
    void insertRows();
};

tst_qquicktreeview::tst_qquicktreeview()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquicktreeview::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 1, 0, "TestModel");
    view = createView();
}

void tst_qquicktreeview::showTreeView()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view is showing the root of the tree
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}


void tst_qquicktreeview::invalidArguments()
{
    // Check that we handle gracefully invalid arguments
    LOAD_TREEVIEW("normaltreeview.qml");

    treeView->expand(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(200);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(200);
    QCOMPARE(treeView->rows(), 1);

    treeView->collapse(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(200);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(200);
    QCOMPARE(treeView->rows(), 1);
}

void tst_qquicktreeview::expandAndCollapseRoot()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    QSignalSpy expandedSpy(treeView, SIGNAL(expanded(int, int)));

    // Expand the root
    treeView->expand(0);

    QCOMPARE(expandedSpy.count(), 1);
    auto signalArgs = expandedSpy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 1);

    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    treeView->collapse(0);
    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeview::toggleExpanded()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    // Expand the root
    treeView->toggleExpanded(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    treeView->toggleExpanded(0);
    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeview::expandAndCollapseChildren()
{
    // Check that we can expand and collapse children, and that
    // the tree ends up with the expected rows
    LOAD_TREEVIEW("normaltreeview.qml");

    const int childCount = 4;
    QSignalSpy expandedSpy(treeView, SIGNAL(expanded(int, int)));

    // Expand the last child of a parent recursively four times
    for (int level = 0; level < 4; ++level) {
        const int nodeToExpand = level * childCount;
        const int firstChildRow = nodeToExpand + 1; // (+ 1 for the root)
        const int lastChildRow = firstChildRow + 4;

        treeView->expand(nodeToExpand);

        QCOMPARE(expandedSpy.count(), 1);
        auto signalArgs = expandedSpy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == nodeToExpand);
        QVERIFY(signalArgs.at(1).toInt() == 1);

        WAIT_UNTIL_POLISHED;

        QCOMPARE(treeView->rows(), lastChildRow);

        auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow))->item;
        QCOMPARE(childItem1->property("text").toString(), "0, 0");
        auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 1))->item;
        QCOMPARE(childItem2->property("text").toString(), "1, 0");
        auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 2))->item;
        QCOMPARE(childItem3->property("text").toString(), "2, 0");
    }

    // Collapse down from level 2 (deliberatly not mirroring the expansion by
    // instead collapsing both level 3 and 4 in one go)
    for (int level = 2; level > 0; --level) {
        const int nodeToCollapse = level * childCount;
        const int firstChildRow = nodeToCollapse - childCount + 1;
        const int lastChildRow = nodeToCollapse + 1; // (+ 1 for the root)

        treeView->collapse(nodeToCollapse);
        WAIT_UNTIL_POLISHED;
        QCOMPARE(treeView->rows(), lastChildRow);

        auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow))->item;
        QCOMPARE(childItem1->property("text").toString(), "0, 0");
        auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 1))->item;
        QCOMPARE(childItem2->property("text").toString(), "1, 0");
        auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 2))->item;
        QCOMPARE(childItem3->property("text").toString(), "2, 0");
    }

    // Collapse the root
    treeView->collapse(0);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(treeView->rows(), 1);
}

void tst_qquicktreeview::requiredPropertiesRoot()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    const auto rootFxItem = treeViewPrivate->loadedTableItem(QPoint(0, 0));
    QVERIFY(rootFxItem);
    const auto rootItem = rootFxItem->item;
    QVERIFY(rootItem);

    const auto context = qmlContext(rootItem.data());
    const auto viewProp = context->contextProperty("treeView").value<QQuickTreeView *>();
    const auto isTreeNode = context->contextProperty("isTreeNode").toBool();
    const auto expanded = context->contextProperty("expanded").toBool();
    const auto hasChildren = context->contextProperty("hasChildren").toBool();
    const auto depth = context->contextProperty("depth").toInt();

    QCOMPARE(viewProp, treeView);
    QCOMPARE(isTreeNode, true);
    QCOMPARE(expanded, false);
    QCOMPARE(hasChildren, true);
    QCOMPARE(depth, 0);

    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    const auto isExpandedAfterExpand = context->contextProperty("expanded").toBool();
    QCOMPARE(isExpandedAfterExpand, true);
}

void tst_qquicktreeview::requiredPropertiesChildren()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    // The last child has it's own children, so expand that one as well
    const auto rootIndex = model->index(0, 0);
    const int childCount = model->rowCount(rootIndex);
    QCOMPARE(childCount, 4);
    const auto lastChildIndex = model->index(childCount - 1, 0, rootIndex);
    QVERIFY(lastChildIndex.isValid());
    QCOMPARE(model->hasChildren(lastChildIndex), true);

    treeView->expand(4);
    WAIT_UNTIL_POLISHED;
    // We now expect root + 4 children + 4 children = 9 rows
    const int rowCount = treeViewPrivate->loadedRows.count();
    QCOMPARE(rowCount, 9);

    // Go through all rows in the view, except the root
    for (int row = 1; row < rowCount; ++row) {
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);

        const auto context = qmlContext(childItem.data());
        const auto viewProp = context->contextProperty("treeView").value<QQuickTreeView *>();
        const auto isTreeNode = context->contextProperty("isTreeNode").toBool();
        const auto expanded = context->contextProperty("expanded").toBool();
        const auto hasChildren = context->contextProperty("hasChildren").toBool();
        const auto depth = context->contextProperty("depth").toInt();

        QCOMPARE(viewProp, treeView);
        QCOMPARE(isTreeNode, true);
        QCOMPARE(expanded, row == 4);
        QCOMPARE(hasChildren, row == 4 || row == 8);
        QCOMPARE(depth, row <= 4 ? 1 : 2);
    }
}

void tst_qquicktreeview::emptyModel()
{
    // Check that you can assign an empty model, and that
    // calling functions will work as expected (and at least not crash)
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->setModel(QVariant());
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeViewPrivate->loadedItems.count(), 0);
    QCOMPARE(treeView->rows(), 0);
    QCOMPARE(treeView->columns(), 0);

    // Check that we don't crash:
    treeView->expand(10);
    treeView->collapse(5);

    QCOMPARE(treeView->depth(0), -1);
    QCOMPARE(treeView->isExpanded(0), false);
    QVERIFY(!treeView->modelIndex(10, 10).isValid());
    QCOMPARE(treeView->rowAtIndex(QModelIndex()), -1);
    QCOMPARE(treeView->columnAtIndex(QModelIndex()), -1);
}

void tst_qquicktreeview::updatedModifiedModel()
{
    // Check that if we change the data in the model, the
    // delegate items get updated.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root plus it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    auto rootFxItem = treeViewPrivate->loadedTableItem(QPoint(0, 0));
    QVERIFY(rootFxItem);
    auto rootItem = rootFxItem->item;
    QVERIFY(rootItem);
    QCOMPARE(rootItem->property("text"), "0, 0");
    model->setData(model->index(0, 0), QVariant(QString("Changed")));
    QCOMPARE(rootItem->property("text"), "Changed");

    auto rootFxItemCol1 = treeViewPrivate->loadedTableItem(QPoint(1, 2));
    QVERIFY(rootFxItemCol1);
    auto rootItemCol1 = rootFxItemCol1->item;
    QVERIFY(rootItemCol1);
    QCOMPARE(rootItemCol1->property("text"), "1, 1");
    model->setData(model->index(1, 1, model->index(0,0)), QVariant(QString("Changed")));
    QCOMPARE(rootItemCol1->property("text"), "Changed");
}

void tst_qquicktreeview::insertRows()
{
    // Check that if we add new rows to the model, TreeView gets updated
    // to contain the new expected number of rows (flattened to a list)
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 5);

    const QModelIndex rootNode = model->index(0, 0, QModelIndex());
    model->insertRows(0, 2, rootNode);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 7);
    auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, 1))->item;
    QCOMPARE(childItem1->property("text").toString(), "0, 0 (inserted)");
    auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, 2))->item;
    QCOMPARE(childItem2->property("text").toString(), "1, 0 (inserted)");
    auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, 3))->item;
    QCOMPARE(childItem3->property("text").toString(), "0, 0");

    const QModelIndex indexOfInsertedChild = model->index(1, 0, rootNode);
    model->insertRows(0, 2, indexOfInsertedChild);
    treeView->expand(2);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 9);
}

void tst_qquicktreeview::expandChildPendingToBeVisible()
{
    // Check that if we expand a row r1, and that row has a child r2 that can
    // be expanded, we can continue to expand c2 immediately, even if r1 is
    // still pending to be shown as expanded in the view.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    QVERIFY(treeView->isExpanded(0));
    // The view has not yet been updated at this point to show
    // the newly expanded children, so it still has only one row.
    QCOMPARE(treeView->rows(), 1);
    // ...but we still expand row 5, which is a child that has children
    // in the proxy model
    treeView->expand(4);
    QVERIFY(treeView->isExpanded(4));
    QCOMPARE(treeView->rows(), 1);

    WAIT_UNTIL_POLISHED;

    // Now the view have updated to show
    // all the rows that has been expanded.
       QCOMPARE(treeView->rows(), 9);
}

void tst_qquicktreeview::expandRecursivelyRoot_data()
{
    QTest::addColumn<int>("rowToExpand");
    QTest::addColumn<int>("depth");

    QTest::newRow("0, -1") << 0 << -1;
    QTest::newRow("0, 0") << 0 << 0;
    QTest::newRow("0, 1") << 0 << 1;
    QTest::newRow("0, 2") << 0 << 2;
}

void tst_qquicktreeview::expandRecursivelyRoot()
{
    // Check that we can expand the root node (row 0), and that
    // all its children are expanded recursively down to the
    // given depth.
    QFETCH(int, rowToExpand);
    QFETCH(int, depth);

    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int, int)));

    treeView->expandRecursively(rowToExpand, depth);

    if (depth == 0) {
        QCOMPARE(spy.count(), 0);
    } else {

        QCOMPARE(spy.count(), 1);
        const auto signalArgs = spy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == rowToExpand);
        QVERIFY(signalArgs.at(1).toInt() == depth);
    }

    WAIT_UNTIL_POLISHED;

    const int rowToExpandDepth = treeView->depth(rowToExpand);
    const int effectiveMaxDepth = depth != -1 ? rowToExpandDepth + depth : model->maxDepth();

    if (depth > 0 || depth == -1)
        QVERIFY(treeView->isExpanded(rowToExpand));
    else
        QVERIFY(!treeView->isExpanded(rowToExpand));

    // Check that all rows after rowToExpand, that are also
    // children of that row, is expanded (down to depth)
    for (int currentRow = rowToExpand + 1; currentRow < treeView->rows(); ++currentRow) {
        const auto modelIndex = treeView->modelIndex(0, currentRow);
        const int currentDepth = treeView->depth(currentRow);
        const bool isChild = currentDepth > rowToExpandDepth;
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        const bool shouldBeExpanded = isChild && isExpandable && currentDepth < effectiveMaxDepth;
        QCOMPARE(treeView->isExpanded(currentRow), shouldBeExpanded);
    }
}

void tst_qquicktreeview::expandRecursivelyChild_data()
{
    QTest::addColumn<int>("rowToExpand");
    QTest::addColumn<int>("depth");

    QTest::newRow("5, -1") << 4 << -1;
    QTest::newRow("5, 0") << 4 << 0;
    QTest::newRow("5, 1") << 4 << 1;
    QTest::newRow("5, 2") << 4 << 2;
    QTest::newRow("5, 3") << 4 << 3;
}

void tst_qquicktreeview::expandRecursivelyChild()
{
    // Check that we can first expand the root node, and the expand
    // recursive the first child node with children (row 4), and that all
    // its children of that node are expanded recursively according to depth.
    QFETCH(int, rowToExpand);
    QFETCH(int, depth);

    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int, int)));

    treeView->expand(0);

    QCOMPARE(spy.count(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 1);

    treeView->expandRecursively(rowToExpand, depth);

    if (depth == 0) {
        QCOMPARE(spy.count(), 0);
    } else {
        QCOMPARE(spy.count(), 1);
        signalArgs = spy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == rowToExpand);
        QVERIFY(signalArgs.at(1).toInt() == depth);
    }

    WAIT_UNTIL_POLISHED;

    const bool rowToExpandDepth = treeView->depth(rowToExpand);
    const int effectiveMaxDepth = depth != -1 ? rowToExpandDepth + depth : model->maxDepth();

    // Check that all rows before rowToExpand is not expanded
    // (except the root node)
    for (int currentRow = 1; currentRow < rowToExpand; ++currentRow)
        QVERIFY(!treeView->isExpanded(currentRow));

    // Check if rowToExpand is expanded
    if (depth > 0 || depth == -1)
        QVERIFY(treeView->isExpanded(rowToExpand));
    else
        QVERIFY(!treeView->isExpanded(rowToExpand));

    // Check that all rows after rowToExpand that is also
    // children of that row is expanded (down to depth)
    for (int currentRow = rowToExpand + 1; currentRow < treeView->rows(); ++currentRow) {
        const int currentDepth = treeView->depth(currentRow);
        const bool isChild = currentDepth > rowToExpandDepth;
        const auto modelIndex = treeView->modelIndex(0, currentRow);
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        const bool shouldBeExpanded = isChild && isExpandable && currentDepth < effectiveMaxDepth;
        QCOMPARE(treeView->isExpanded(currentRow), shouldBeExpanded);
    }
}

void tst_qquicktreeview::expandRecursivelyWholeTree()
{
    // Check that we expand the whole tree recursively by passing -1, -1
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int, int)));
    treeView->expandRecursively(-1, -1);

    QCOMPARE(spy.count(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toInt() == -1);

    WAIT_UNTIL_POLISHED;

    // Check that all rows that have children are expanded
    for (int currentRow = 0; currentRow < treeView->rows(); ++currentRow) {
        const auto modelIndex = treeView->modelIndex(0, currentRow);
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        QCOMPARE(treeView->isExpanded(currentRow), isExpandable);
    }
}

void tst_qquicktreeview::collapseRecursivelyRoot()
{
    // Check that we can collapse the root node (row 0), and that
    // all its children are collapsed recursively down to the leaves.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expandRecursively();
    WAIT_UNTIL_POLISHED;

    // Verify that the tree is now fully expanded
    const int expectedRowCount = 1 + (model->maxDepth() * 4); // root + 4 children per level
    QCOMPARE(treeView->rows(), expectedRowCount);

    QSignalSpy spy(treeView, SIGNAL(collapsed(int, bool)));

    // Collapse the whole tree again. This time, only the root should end up visible
    treeView->collapseRecursively();

    QCOMPARE(spy.count(), 1);
    const auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 1);

    // We need to check that all descendants are collapsed as well. But since they're
    // now no longer visible in the view, we need to expand each parent one by one again to make
    // them visible, and check that the child inside that has children is still collapsed.
    // We can do that by simply iterate over the rows in the view as we expand.
    int currentRow = 0;
    while (currentRow < treeView->rows()) {
        const QModelIndex currentIndex = treeView->modelIndex(0, currentRow);
        if (model->hasChildren(currentIndex)) {
            QVERIFY(!treeView->isExpanded(currentRow));
            treeView->expand(currentRow);
            WAIT_UNTIL_POLISHED;
        }
        currentRow++;
    }

    // Sanity check that we ended up with all rows expanded again
    QCOMPARE(currentRow, expectedRowCount);
}

void tst_qquicktreeview::collapseRecursivelyChild()
{
    // Check that we can collapse a child node (row 4), and that all its children
    // are collapsed recursively down to the leaves (without touching the root).
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expandRecursively();
    WAIT_UNTIL_POLISHED;

    // Verify that the tree is now fully expanded
    const int expectedRowCount = 1 + (model->maxDepth() * 4); // root + 4 children per level
    QCOMPARE(treeView->rows(), expectedRowCount);

    QSignalSpy spy(treeView, SIGNAL(collapsed(int, bool)));

    // Collapse the 4th child recursive
    const int rowToCollapse = 4;
    QCOMPARE(model->data(treeView->modelIndex(0, rowToCollapse), Qt::DisplayRole), QStringLiteral("3, 0"));
    treeView->collapseRecursively(rowToCollapse);

    QCOMPARE(spy.count(), 1);
    const auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == rowToCollapse);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 5); // root + 4 children

    // We need to check that all descendants are collapsed as well. But since they're
    // now no longer visible in the view, we need to expand each parent one by one again to make
    // them visible, and check that the child inside that has children is still collapsed.
    // We can do that by simply iterate over the rows in the view as we expand.
    int currentRow = 1; // start at first child
    while (currentRow < treeView->rows()) {
        const QModelIndex currentIndex = treeView->modelIndex(0, currentRow);
        if (model->hasChildren(currentIndex)) {
            QVERIFY(!treeView->isExpanded(currentRow));
            treeView->expand(currentRow);
            WAIT_UNTIL_POLISHED;
        }
        currentRow++;
    }

    // Sanity check that we ended up with all rows expanded again
    QCOMPARE(currentRow, expectedRowCount);
}

void tst_qquicktreeview::collapseRecursivelyWholeTree()
{
    // Check that we collapse the whole tree recursively by passing -1
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(collapsed(int, bool)));
    treeView->expandRecursively();
    treeView->collapseRecursively();

    QCOMPARE(spy.count(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 1); // root
}

void tst_qquicktreeview::expandToIndex()
{
    // Check that expandToIndex(index) expands the tree so
    // that index becomes visible in the view
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int, int)));

    const QModelIndex root = model->index(0, 0);
    const QModelIndex child1 = model->index(3, 0, root);
    const QModelIndex child2 = model->index(3, 0, child1);

    QVERIFY(model->hasChildren(root));
    QVERIFY(model->hasChildren(child1));
    QVERIFY(model->hasChildren(child2));

    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(root)));
    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(child1)));
    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(child2)));

    const QModelIndex childToExpand = model->index(1, 0, child2);
    treeView->expandToIndex(childToExpand);

    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(root)));
    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(child1)));
    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(child2)));

    QCOMPARE(spy.count(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 3);

    WAIT_UNTIL_POLISHED;

    // The view should now have 13 rows:
    // root + 3 expanded nodes that each have 4 children
    QCOMPARE(treeView->rows(), 13);
}


QTEST_MAIN(tst_qquicktreeview)

#include "tst_qquicktreeview.moc"
