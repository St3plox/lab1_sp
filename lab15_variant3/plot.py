"""Графики последнего запуска: все подписи читаются из сохранённого CSV."""
import csv
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # Работает без графического окна.
import matplotlib.pyplot as plt

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("results/timings.csv")
with source.open(newline="") as file:
    rows = list(csv.DictReader(file))
if not rows:
    raise SystemExit("В CSV нет измерений. Сначала запустите ./points.")
rows.sort(key=lambda row: int(row["elements"]))

settings = {(r["threads"], r["repeats"], r.get("distance", "50"),
             r.get("input_mode", "generated"), r.get("seed", "12345")) for r in rows}
if len(settings) != 1:
    raise SystemExit("В файле смешаны разные настройки. Используйте CSV одной серии.")
threads, repeats, distance, mode, seed = settings.pop()
n = [int(r["elements"]) for r in rows]
serial = [float(r["serial_ms"]) for r in rows]
parallel = [float(r["parallel_ms"]) for r in rows]
speedup = [s / p for s, p in zip(serial, parallel)]
input_description = "ручной ввод" if mode == "manual" else f"генератор, seed={seed}"
metadata = f"Потоков: {threads}; R={float(distance):g}; медиана {repeats} повторов; {input_description}"
if len(rows) == 1:
    r = rows[0]
    metadata += f"\nМатрица {r['rows']} × {r['cols']}; точек: {r['points']}; найдено: {r.get('serial_count', r.get('count'))}"
else:
    metadata += f"\nРазмеров матрицы: {len(rows)}; подписи точек: строки × столбцы"
if "timestamp" in rows[0]:
    metadata += f"\nЗапуск: {rows[0]['timestamp']}"

first_faster = next((i for i, (s, p) in enumerate(zip(serial, parallel)) if p < s), None)
if len(rows) == 1:
    conclusion = ("Измерен только один размер матрицы. Определить зависимость времени "
                  "от размера и первый размер с выигрышем нельзя. "
                  "Запустите make analysis для серии размеров.")
elif first_faster is None:
    conclusion = "Среди исследованных размеров параллельный способ не оказался быстрее."
else:
    i = first_faster
    r = rows[i]
    conclusion = (f"Первый выигрыш среди исследованных размеров: {n[i]:,} элементов "
                  f"({r['rows']} × {r['cols']}, {r['points']} точек). "
                  f"Последовательно: {serial[i]:.6f} мс; параллельно: {parallel[i]:.6f} мс; "
                  f"ускорение: {speedup[i]:.3f}.")
conclusion += ("\nЭто результат для указанного числа потоков, текущей машины и нагрузки; "
               "точная граница между проверенными размерами не измерялась. "
               "В обоих способах число операций растёт линейно: O(N).")

fig, axes = plt.subplots(1, 2, figsize=(13, 6))
fig.subplots_adjust(top=0.78, bottom=0.14, left=0.085, right=0.96, wspace=0.3)
fig.suptitle(metadata, fontsize=11, y=0.98)
if len(rows) == 1:
    bars = axes[0].bar(["Последовательно", f"Параллельно\nпотоков: {threads}"],
                       [serial[0], parallel[0]], color=["#287ab3", "#ef8a21"])
    axes[0].bar_label(bars, fmt="%.6f")
    axes[0].set_ylim(0, max(serial[0], parallel[0]) * 1.25)
    bars = axes[1].bar([f"Потоков: {threads}"], speedup, color="#237c55")
    axes[1].bar_label(bars, fmt="%.3f")
    axes[1].set_ylim(0, max(1, speedup[0]) * 1.25)
    axes[0].set_ylabel("Время, мс")
else:
    axes[0].plot(n, serial, "o-", label="Последовательно")
    axes[0].plot(n, parallel, "s-", label=f"Параллельно (потоков: {threads})")
    axes[0].set_xscale("log")
    axes[0].set_yscale("log")
    axes[0].set_xlabel("Элементов матрицы (2 элемента = 1 точка)")
    axes[0].set_ylabel("Время, мс (логарифмическая шкала)")
    axes[0].legend()
    axes[1].plot(n, speedup, "o-", color="#237c55", label=f"Потоков: {threads}")
    axes[1].set_xscale("log")
    axes[1].set_xlabel("Количество элементов матрицы")
    if first_faster is not None:
        for axis in axes:
            axis.axvline(n[first_faster], color="#7755aa", linestyle=":", alpha=0.8)
        axes[1].text(0.97, 0.04, f"Первый выигрыш: N = {n[first_faster]:,}",
                     transform=axes[1].transAxes, ha="right", fontsize=9,
                     bbox=dict(facecolor="white", edgecolor="#7755aa", alpha=0.9))
    for index, (r, x, y) in enumerate(zip(rows, n, parallel)):
        axes[0].annotate(f"{r['rows']}×{r['cols']}", (x, y),
                         xytext=(0, 8), textcoords="offset points", fontsize=7,
                         ha="right" if index == len(rows) - 1 else "left")
axes[0].set_title("Время обработки")
axes[1].axhline(1, color="#aa3333", linestyle="--", label="Равное время")
axes[1].set_ylabel("Последовательное время / параллельное время")
axes[1].set_title("Ускорение: выше 1 — параллельно быстрее")
axes[1].legend()
for axis in axes:
    axis.grid(True, which="both", alpha=0.25)
    axis.set_axisbelow(True)

fig.savefig(source.with_suffix(".png"), dpi=180)
fig.savefig(source.with_suffix(".svg"))
if source == Path("results/timings.csv"):
    fig.savefig("results/timing.png", dpi=180)
    fig.savefig("results/timing.svg")
plt.close(fig)
report = source.with_suffix(".txt")
report.write_text(metadata + "\n\n" + conclusion + "\n", encoding="utf-8")
print(f"Графики: {source.with_suffix('.png')} и {source.with_suffix('.svg')}")
print(conclusion)
print(f"Сохранённый вывод: {report}")
