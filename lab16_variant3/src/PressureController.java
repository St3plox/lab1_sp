/** Общий контракт задатчиков давления. Давление задаётся в избыточных атмосферах. */
public abstract class PressureController {
    /**
     * Возвращает описание сигналов. Недопустимое или недостижимое для данного
     * устройства давление отклоняется через IllegalArgumentException.
     */
    public final String createSignals(double pressure) {
        if (!Double.isFinite(pressure) || pressure < 0) {
            throw new IllegalArgumentException("Давление должно быть конечным и неотрицательным.");
        }
        return calculateSignals(pressure);
    }

    protected abstract String calculateSignals(double pressure);
}
