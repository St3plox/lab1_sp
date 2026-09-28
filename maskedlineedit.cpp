#include "maskedlineedit.h" // Объявление класса, методы которого реализуем ниже.

#include <QValidator> // Базовый класс Qt для проверки вводимого текста.

#include <stdexcept> // std::invalid_argument: ошибка при неверной маске.
#include <utility>   // std::move: перемещение строки без лишней копии.

// Безымянное пространство имён скрывает MaskValidator от других .cpp-файлов.
// Это внутренний помощник, поэтому его нет в maskedlineedit.h.
namespace {
// Наследуем QValidator, чтобы Qt вызывал нашу проверку при вводе в поле.
class MaskValidator final : public QValidator {
public:
    // QObject *parent передаём базовому классу: Qt удалит валидатор вместе
    // с родительским полем, поэтому вручную вызывать delete не нужно.
    explicit MaskValidator(QString mask, QObject *parent)
        : QValidator(parent), mask_(std::move(mask)) {}

    // override: заменяем виртуальный метод QValidator::validate своей логикой.
    // input — проверяемый текст; position — позиция курсора (нам не нужна).
    // Qt ожидает одно из состояний: Invalid, Intermediate, Acceptable.
    State validate(QString &input, int &position) const override {
        Q_UNUSED(position)
        // Больше символов, чем мест в маске, быть не может.
        if (input.size() > mask_.size())
            return Invalid;

        // Длина совпала с маской — возможно, ввод завершён. Ниже проверим
        // каждый символ: незаполненные позиции '_' изменят флаг на false.
        bool complete = input.size() == mask_.size();
        // qsizetype — тип, которым Qt измеряет длину строк и контейнеров.
        for (qsizetype i = 0; i < input.size(); ++i) {
            const QChar actual = input[i]; // Символ в текущей позиции.
            // '_' — показываемый Qt заполнитель пустой позиции.
            // Он допустим временно вместо цифры или буквы, но не вместо '-'.
            if (actual == QLatin1Char('_') && mask_[i] != QLatin1Char('-')) {
                complete = false;
                continue; // Переходим к следующему символу цикла.
            }

            bool matches = false; // Пока считаем текущий символ неподходящим.
            // toLatin1() здесь безопасен: маска состоит только из 9, a, -.
            switch (mask_[i].toLatin1()) {
            // Для 9 разрешены только символы от '0' до '9'.
            case '9': matches = actual >= QLatin1Char('0') && actual <= QLatin1Char('9'); break;
            // Для a разрешены только латинские буквы обоих регистров.
            case 'a': matches = (actual >= QLatin1Char('A') && actual <= QLatin1Char('Z')) ||
                                 (actual >= QLatin1Char('a') && actual <= QLatin1Char('z')); break;
            // Минус должен стоять именно там, где он указан в маске.
            case '-': matches = actual == QLatin1Char('-'); break;
            }
            // Например, кириллическая буква вместо a сразу отклоняется.
            if (!matches)
                return Invalid;
        }
        // Полный корректный ввод принимаем. Корректный, но неполный,
        // оставляем в состоянии Intermediate, чтобы пользователь мог дописать.
        return complete ? Acceptable : Intermediate;
    }

private:
    QString mask_; // Копия исходной маски для посимвольного сравнения.
};
} // Конец безымянного пространства имён.

// «: QLineEdit(parent)» сначала вызывает конструктор исходного класса Qt.
// «mask_(mask)» сохраняет переданную строку в поле нашего класса.
MaskedLineEdit::MaskedLineEdit(const QString &mask, QWidget *parent)
    : QLineEdit(parent), mask_(mask) {
    // Пустая маска не имеет смысла: сообщаем вызывающему коду об ошибке.
    if (mask_.isEmpty())
        throw std::invalid_argument("Mask must not be empty");

    QString qtMask; // Та же маска, но в специальном синтаксисе QLineEdit.
    // range-based for перебирает символы исходной маски по одному.
    for (const QChar ch : mask_) {
        if (ch == QLatin1Char('9'))
            qtMask += QLatin1Char('0'); // В синтаксисе Qt 0 = обязательная цифра.
        else if (ch == QLatin1Char('a'))
            qtMask += QLatin1Char('A'); // В синтаксисе Qt A = обязательная буква.
        else if (ch == QLatin1Char('-'))
            qtMask += QStringLiteral("\\-"); // Явно помечаем минус как буквальный символ.
        else
            // Разрешены только три символа, указанные в задании.
            throw std::invalid_argument("Mask may contain only 9, a, and -");
    }
    // После ';' в маске Qt задаётся символ незаполненной позиции.
    // Например, «999» превращается в «000;_» и сначала выглядит как «___».
    qtMask += QStringLiteral(";_");

    // Метод QLineEdit расставляет позиции и постоянные символы (например, '-').
    setInputMask(qtMask);
    // Дополнительная проверка нужна, чтобы a значило именно латиницу,
    // а 9 — именно ASCII-цифру: маска Qt сама допускает иные письменности.
    // this — указатель на текущее поле, оно станет родителем валидатора.
    setValidator(new MaskValidator(mask_, this));
    // Qt добавит кнопку очистки в правую часть поля, когда там есть текст.
    setClearButtonEnabled(true);
}

// hasAcceptableInput() — унаследованный метод QLineEdit. Он учитывает и
// маску ввода, и установленный выше валидатор.
bool MaskedLineEdit::isComplete() const {
    return hasAcceptableInput();
}

// Тернарный оператор «условие ? A : B» выбирает один из двух результатов.
// text() — унаследованный метод QLineEdit, возвращающий введённую строку.
QString MaskedLineEdit::value() const {
    return isComplete() ? text() : QString();
}
