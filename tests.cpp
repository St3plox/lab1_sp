#include "maskedlineedit.h"

#include <QClipboard>
#include <QtTest>

class MaskedLineEditTest : public QObject {
    Q_OBJECT

private slots:
    void digits();
    void lettersAndLiteral();
    void badMask();
};

void MaskedLineEditTest::digits() {
    MaskedLineEdit field(QStringLiteral("999"));
    field.show();
    field.setFocus();
    QTest::keyClicks(&field, "00x7");
    QCOMPARE(field.value(), QStringLiteral("007"));
    QVERIFY(field.isComplete());
    field.clear();
    QTest::keyClicks(&field, "12");
    QVERIFY(!field.isComplete());
    QVERIFY(field.value().isEmpty());
}

void MaskedLineEditTest::lettersAndLiteral() {
    MaskedLineEdit field(QStringLiteral("aa-9"));
    field.show();
    field.setFocus();
    QTest::keyClicks(&field, "Ab4");
    QCOMPARE(field.value(), QStringLiteral("Ab-4"));
    field.clear();
    QApplication::clipboard()->setText(QString::fromUtf8("Яb4"));
    field.paste();
    QVERIFY(!field.isComplete());
}

void MaskedLineEditTest::badMask() {
    QVERIFY_EXCEPTION_THROWN(MaskedLineEdit{QString()}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(MaskedLineEdit(QStringLiteral("9x")), std::invalid_argument);
}

QTEST_MAIN(MaskedLineEditTest)
#include "tests.moc"
