#include "maskedlineedit.h" // Проверяемый класс.

#include <QClipboard> // Буфер обмена для проверки вставки текста.
#include <QtTest>     // Средства Qt для имитации ввода и проверок.

// QObject и Q_OBJECT нужны, чтобы Qt Test нашёл функции проверки (слоты).
class MaskedLineEditTest : public QObject {
    Q_OBJECT

private slots: // Qt Test автоматически запустит каждую функцию ниже.
    void digits();            // Проверка цифровой маски.
    void lettersAndLiteral(); // Проверка букв и постоянного минуса.
    void badMask();           // Проверка неверных масок.
};

void MaskedLineEditTest::digits() {
    MaskedLineEdit field(QStringLiteral("999")); // Три обязательные цифры.
    field.show();     // Для имитации клавиш поле должно быть показано.
    field.setFocus(); // Направляем ввод клавиатуры именно в это поле.
    // x не соответствует 9 и должен быть отброшен; результат — 007.
    QTest::keyClicks(&field, "00x7");
    QCOMPARE(field.value(), QStringLiteral("007")); // Сравниваем факт с ожиданием.
    QVERIFY(field.isComplete()); // Проверяем, что поле целиком заполнено.
    field.clear(); // Начинаем второй опыт с пустого поля.
    QTest::keyClicks(&field, "12"); // Ввели только две цифры из трёх.
    QVERIFY(!field.isComplete());
    QVERIFY(field.value().isEmpty()); // Незаконченное значение не возвращается.
}

void MaskedLineEditTest::lettersAndLiteral() {
    MaskedLineEdit field(QStringLiteral("aa-9")); // Две буквы, минус, цифра.
    field.show();
    field.setFocus();
    QTest::keyClicks(&field, "Ab4"); // Минус подставляется Qt автоматически.
    QCOMPARE(field.value(), QStringLiteral("Ab-4"));
    field.clear();
    // Через буфер проверяем кириллицу: QTest::keyClicks принимает ASCII.
    QApplication::clipboard()->setText(QString::fromUtf8("Яb4"));
    field.paste(); // Имитируем обычную вставку пользователя.
    QVERIFY(!field.isComplete()); // Кириллица не должна дать готовое значение.
}

void MaskedLineEditTest::badMask() {
    // Конструктор должен бросить исключение, если маска недопустима.
    QVERIFY_EXCEPTION_THROWN(MaskedLineEdit{QString()}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(MaskedLineEdit(QStringLiteral("9x")), std::invalid_argument);
}

QTEST_MAIN(MaskedLineEditTest) // Qt создаёт функцию main для программы тестов.
#include "tests.moc" // Подключаем код, сгенерированный Qt для Q_OBJECT.
