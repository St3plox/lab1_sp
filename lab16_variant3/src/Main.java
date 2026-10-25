import java.util.Locale;

public class Main {
    public static void main(String[] args) {
        // Один массив базового типа: вызов метода не зависит от реализации драйвера.
        PressureController[] controllers = {
            new ValvePressureController(10.0, 0.0),
            new PressPressureController(1.0, 10.0, 0.5)
        };

        try {
            double[] pressures = {1.0, 3.0, 5.0};
            if (args.length > 0) {
                pressures = new double[args.length];
                for (int i = 0; i < args.length; i++) {
                    pressures[i] = Double.parseDouble(args[i]);
                }
            }
            for (double pressure : pressures) {
                System.out.printf(Locale.ROOT, "Требуемое давление: %.6f атм%n", pressure);
                for (PressureController controller : controllers) {
                    System.out.println(controller.createSignals(pressure));
                }
                System.out.println();
            }
        } catch (IllegalArgumentException e) {
            System.err.println("Ошибка: " + e.getMessage());
            System.exit(1);
        }
    }
}
