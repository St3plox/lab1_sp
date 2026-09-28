#include "maskedlineedit.h" // Наш новый класс поля ввода по маске.

#include <QApplication> // Объект приложения и цикл обработки событий.
#include <QFormLayout>  // Размещает пары «подпись — поле ввода».
#include <QLabel>       // Надписи с текстом.
#include <QPushButton>  // Кнопка.
#include <QVBoxLayout>  // Размещает элементы вертикально.
#include <QWidget>      // Базовый виджет, здесь — главное окно.

// main — точка входа в программу. argc и argv содержат параметры запуска.
int main(int argc, char *argv[]) {
    // QApplication должен существовать до создания любых виджетов Qt.
    QApplication app(argc, argv);

    // Главное окно создаём как локальную переменную: оно удалится при выходе
    // из main. Виджеты, помещённые в окно, Qt удалит вместе с ним.
    QWidget window;
    window.setWindowTitle(QStringLiteral("Ввод по маске — вариант 3"));
    window.setMinimumWidth(430); // Чтобы подписи и поля не были слишком узкими.

    // new создаёт объект в динамической памяти и возвращает указатель на него.
    // Переданный &window задаёт родителя компоновки (адрес нашего окна).
    auto *layout = new QVBoxLayout(&window);
    // auto просит компилятор вывести тип переменной из new QLabel(...).
    auto *title = new QLabel(QStringLiteral("Поле ввода по маске"));
    // Берём обычный шрифт надписи и делаем заголовок крупнее и жирнее.
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    // Компоновка добавляет виджеты в окно по порядку, сверху вниз.
    layout->addWidget(title);
    layout->addWidget(new QLabel(QStringLiteral("9 — цифра, a — латинская буква, - — знак минус.")));

    // Форма удобна для подписей слева и соответствующих полей справа.
    auto *form = new QFormLayout;
    // Создаём три объекта ОДНОГО класса, но с разными масками конструктора.
    auto *digits = new MaskedLineEdit(QStringLiteral("999"));
    auto *code = new MaskedLineEdit(QStringLiteral("aaa-999"));
    auto *mixed = new MaskedLineEdit(QStringLiteral("99-aa"));
    // addRow связывает видимую подпись с полем ввода.
    form->addRow(QStringLiteral("999:"), digits);
    form->addRow(QStringLiteral("aaa-999:"), code);
    form->addRow(QStringLiteral("99-aa:"), mixed);
    layout->addLayout(form); // Вставляем всю форму в вертикальную компоновку.

    auto *status = new QLabel; // Здесь покажем полностью введённые значения.
    status->setWordWrap(true); // Разрешаем переносить длинную строку.
    layout->addWidget(status); // Показываем статус под полями.

    // Лямбда — небольшая функция без отдельного имени. [=] сохраняет
    // использованные указатели на поля и надпись, чтобы обращаться к ним позже.
    const auto updateStatus = [=] {
        QStringList ready; // Список строк с готовыми значениями.
        // Перебираем все три поля. auto *field — указатель на очередное поле.
        for (auto *field : {digits, code, mixed}) {
            // Неполное поле пока не показываем как готовый результат.
            if (field->isComplete())
                // Оператор -> вызывает метод через указатель на объект.
                ready << field->mask() + QStringLiteral(": ") + field->value();
        }
        // Условие ? A : B выбирает сообщение для пустого/непустого списка.
        // join("; ") объединяет готовые значения через точку с запятой.
        status->setText(ready.isEmpty()
            ? QStringLiteral("Заполните поля. Неверные символы не вводятся.")
            : QStringLiteral("Корректные значения: ") + ready.join(QStringLiteral("; ")));
    };
    // Сигнал textChanged унаследован от QLineEdit. Он возникает при изменении
    // текста. connect сообщает Qt: при сигнале вызови updateStatus.
    // &window задаёт контекст: связь удалится при уничтожении окна.
    for (auto *field : {digits, code, mixed})
        QObject::connect(field, &QLineEdit::textChanged, &window, updateStatus);
    updateStatus(); // Показываем начальное сообщение до первого ввода.

    // Кнопка вызывает ещё одну лямбду при сигнале clicked.
    auto *clear = new QPushButton(QStringLiteral("Очистить поля"));
    QObject::connect(clear, &QPushButton::clicked, &window, [=] {
        // clear() и setFocus() — унаследованные методы QLineEdit/QWidget.
        for (auto *field : {digits, code, mixed})
            field->clear();
        digits->setFocus(); // Курсор снова попадает в первое поле.
        updateStatus();     // Обновляем сообщение после очистки.
    });
    layout->addWidget(clear); // Помещаем кнопку под статусом.

    window.show(); // Делаем созданное окно видимым.
    return app.exec(); // Запускаем цикл Qt: ждём ввод, клики и закрытие окна.
}
