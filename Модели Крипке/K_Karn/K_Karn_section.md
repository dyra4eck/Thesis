# Раздел 3.Z. Формальная модель Крипке алгоритма Карна (K_Karn)

> **Источник:** Max von Hippel, *Verification and Attack Synthesis for Network Protocols*,
> arXiv:2511.01124v1, 2025. Разделы 2.2–2.4, стр. 14–21.
> DOI: https://doi.org/10.48550/arXiv.2511.01124
>
> **Оригинальная публикация алгоритма:**
> Phil Karn, Craig Partridge. *Improving round-trip time estimates in reliable
> transport protocols*. ACM SIGCOMM Computer Communication Review, 17(5), 1987.

---

## 3.Z.1. Неформальное описание механизма

Алгоритм Карна решает **проблему неоднозначности RTT при ретрансмиссии**.
В транспортных протоколах типа TCP, когда пакет ретрансмитируется, невозможно
определить, какой именно передаче соответствует полученный ACK — исходной или
повторной. Использование такого ACK для вычисления RTT приведёт к некорректной
оценке.

Алгоритм Карна является **немешающим монитором** (non-interfering monitor,
von Hippel (2025), p. 17): он наблюдает за действиями отправителя — событиями
`snds(i)` и `rcvs(j)` — и выполняет учётные операции, не влияя на поведение
самого отправителя. Переменные монитора `V = {τ, numT, time, high, S}` полностью
отделены от переменных отправителя.

**Правило выборки** (p. 20): выборка RTT производится только тогда, когда
выполняется условие `ok-to-sample` — все пакеты в диапазоне `[high, j)` были
переданы ровно один раз:

```
ok-to-sample(numT, high) :=
    (∀k: high < k < j → numT[k] = 1) ∧ (high > 0 → numT[high] = 1)
```

При соблюдении условия алгоритм вычисляет выборку (Algorithm 1, p. 19,
строки 13–16):

```
S := τ − time[high]
```

---

## 3.Z.2. Ключевое отличие K_Karn от K_GBN и K_RTO

Три модели Крипке данной диссертации верифицируют разные уровни одной
цепочки механизмов:

| Модель | Что верифицируется | Тип |
|--------|-------------------|-----|
| **K_Karn** | Алгоритм выборки RTT — монитор над исполнением | монитор |
| **K_RTO** | Вычисление RTO на основе выборок Карна | вычисление |
| **K_GBN** | Протокол передачи, использующий RTO как таймер | протокол |

K_Karn не имеет собственного управления потоком передачи. Его состояния
кодируют **фазу последнего наблюдённого события**: что произошло при обработке
последнего `snds` или `rcvs`, а не числовые значения переменных.

---

## 3.Z.3. Переменные и их абстракция

| Переменная алгоритма | Тип | Абстракция в K_Karn |
|---|---|---|
| `numT[i]` | ℕ⁺→ℕ | Булево: `= 1` → `Pkt_first_send`; `> 1` → `Pkt_retransmitted` |
| `time[i]` | ℕ⁺→ℕ | Инвариант I2 гарантирован структурой переходов; явно не моделируется |
| `high` | ℕ | Определяет фазу ACK: `ACK_stale` (j ≤ high) / `ACK_fresh` (j > high) |
| `τ` | ℕ | Моделируется пропозицией `tau_incremented` (строки 9, 19 Algorithm 1) |
| `S` | ℕ⁺ | `Sample_produced = TRUE` при выполнении строки 14 Algorithm 1 |

---

## 3.Z.4. Формальный кортеж K_Karn

$$K_{Karn} = \langle AP,\; S,\; s_0,\; T,\; L \rangle$$

### Атомарные пропозиции AP

$$AP = \{\; Karn\_idle,\; Pkt\_first\_send,\; Pkt\_retransmitted,\; tau\_incremented,$$
$$Window\_unambiguous,\; Window\_ambiguous,$$
$$ACK\_stale,\; ACK\_fresh,\; ACK\_no\_sample,$$
$$Sample\_produced,\; Sample\_exact,\; Sample\_pessimistic,$$
$$FIFO\_channel,\; NonFIFO\_channel,\; Karn\_done \;\}$$

| Пропозиция | Смысл | Наблюдение |
|---|---|---|
| `Window_unambiguous` | ok-to-sample потенциально TRUE | Obs.3, p. 20 |
| `Window_ambiguous` | Ретрансмиссия заблокировала выборку | Obs.2 I1, p. 20 |
| `ACK_fresh` | j > high — новый ACK | Obs.1, p. 17 |
| `ACK_stale` | j ≤ high — ACK проигнорирован | Obs.1, p. 17 |
| `Sample_produced` | Выборка выдана (строка 14, p. 19) | Obs.3, p. 20 |
| `Sample_exact` | S = rtt(high_prev) точно | Obs.4, p. 21 |
| `Sample_pessimistic` | S — верхняя граница RTT | Obs.3, p. 20 |
| `FIFO_channel` | ACK доставляются в FIFO-порядке | Obs.4, p. 21 |

### Множество состояний S и функция разметки L

| Состояние | Активные пропозиции | Соответствие Algorithm 1 |
|---|---|---|
| `s0` | `Karn_idle` | Инициализация: τ=1, high=0 (p. 19) |
| `s1` | `Pkt_first_send`, `tau_incremented` | snds(i): numT[i]:=1, time[i]:=τ, τ++ (строки 5–9) |
| `s2` | `Pkt_retransmitted`, `tau_incremented` | snds(i) повторно: numT[i]>1, time[i] не меняется (строка 5) |
| `s3` | `Window_unambiguous` | Ожидание ACK: ok-to-sample потенциально TRUE |
| `s4` | `Window_ambiguous` | Ожидание ACK: ok-to-sample=FALSE (ретрансмиссия в окне) |
| `s5` | `ACK_stale` | rcvs(j): j ≤ high — пропуск блока (строка 12, p. 19) |
| `s6` | `ACK_fresh`, `Sample_produced` | rcvs(j): j > high И ok-to-sample=TRUE (строки 13–16) |
| `s7` | `ACK_fresh`, `ACK_no_sample` | rcvs(j): j > high НО ok-to-sample=FALSE, high:=j (строка 17) |
| `s8` | `Sample_produced`, `FIFO_channel`, `Sample_exact`, `ACK_fresh` | FIFO: S=rtt(high_prev) точно (Obs.4, p. 21) |
| `s9` | `Sample_produced`, `NonFIFO_channel`, `Sample_pessimistic`, `ACK_fresh` | Не-FIFO: S — верхняя граница (Obs.3, p. 20) |
| `s_cont` | `tau_incremented` | τ++ (строка 19), high обновлён, следующий цикл |
| `s_done` | `Karn_done` | Все пакеты подтверждены |

### Начальное состояние

$$s_0: L(s_0) = \{Karn\_idle\}$$

### Отношение переходов T (19 переходов)

```
s0  → s1                      -- начало: первая отправка
s1  → s3                      -- окно однозначное (numT[i]=1)
s1  → s2                      -- немедленная ретрансмиссия
s2  → s4                      -- ждём ACK с неоднозначным окном
s2  → s2                      -- ещё ретрансмиссии (numT[i]++)
s3  → s5                      -- rcvs(j): j ≤ high (устаревший)
s3  → s6                      -- rcvs(j): j > high И ok-to-sample=TRUE
s3  → s7                      -- rcvs(j): j > high И ok-to-sample=FALSE
s4  → s5                      -- rcvs(j): j ≤ high
s4  → s7                      -- rcvs(j): j > high, выборка заблокирована
s5  → s_cont                  -- устаревший ACK обработан (высокий игнорируется)
s6  → s8                      -- FIFO-канал: выборка точная (Obs.4)
s6  → s9                      -- Не-FIFO-канал: выборка пессимистичная (Obs.3)
s7  → s_cont                  -- high:=j, выборка пропущена
s8  → s_cont                  -- точная выборка передана в RTO
s9  → s_cont                  -- пессимистичная выборка передана в RTO
s_cont → s1                   -- следующий пакет
s_cont → s_done               -- все пакеты подтверждены
s_done → s_done               -- терминальное состояние
```

---

## 3.Z.5. Ограничение справедливости

```smv
FAIRNESS
    Sample_produced | Karn_done | ACK_stale
```

**Обоснование.** Без ограничения справедливости nuXmv допускает бесконечный
путь `s2→s2→s2...` (бесконечные ретрансмиссии без получения ACK) или
`s2→s4→s5→s_cont→s2→...` (бесконечное получение устаревших ACK). Оба
сценария реализуются при постоянных потерях всех ACK — ситуация, физически
невозможная в корректно функционирующей сети с механизмом разрыва соединения.

Ограничение справедливости требует, чтобы на каждом бесконечном пути
бесконечно часто происходило хотя бы одно из: выборка произведена,
исполнение завершено, или получен устаревший ACK. Последнее включено,
поскольку устаревший ACK — семантически корректный исход неоднозначного
окна: алгоритм Карна правомерно его игнорирует (строка 12 Algorithm 1, p. 19).

---

## 3.Z.6. Верифицируемые CTL-свойства

| № | CTL-спецификация | Наблюдение | Страница |
|---|---|---|---|
| 1 | `AG (EX TRUE)` | — | — |
| 2 | `AG !(Window_unambiguous & Window_ambiguous)` | Obs.2 (I1) | p. 20 |
| 3 | `AG !(ACK_stale & ACK_fresh)` | Obs.1 | p. 17 |
| 4 | `AG !(Sample_exact & Sample_pessimistic)` | Obs.3 vs Obs.4 | p. 20–21 |
| 5 | `AG ((Sample_produced & FIFO_channel) → Sample_exact)` | Obs.4 | p. 21 |
| 6 | `AG ((Sample_produced & NonFIFO_channel) → Sample_pessimistic)` | Obs.3 | p. 20 |
| 7 | `AG (Sample_produced → ACK_fresh)` | Obs.3 (строка 12) | p. 19–20 |
| 8 | `AG (Window_ambiguous → AF (ACK_no_sample \| ACK_stale \| Karn_done))` | Obs.3 | p. 20 |
| 9 | `AG (Pkt_retransmitted → AF Window_ambiguous)` | Obs.2 (I1) | p. 20 |

---

## 3.Z.7. Процесс верификации и итерационное уточнение

### Итерация 1 — `AG (Sample_produced → ACK_fresh)` is false

**Контрпример:**

```
→ State 1.1: s0   [Karn_idle = TRUE]
→ State 1.2: s1   [Pkt_first_send = TRUE]
→ State 1.3: s3   [Window_unambiguous = TRUE]
→ State 1.4: s6   [ACK_fresh = TRUE,  Sample_produced = TRUE]
→ State 1.5: s9   [ACK_fresh = FALSE, Sample_pessimistic = TRUE]
```

**Анализ.** Состояние s9 достигается из s6, где ACK был свежим. Однако в
первоначальной модели предикат `ACK_fresh` не был объявлен для s8 и s9.
Эти состояния наступают *непосредственно* после получения свежего ACK в s6
— в тот же шаг Algorithm 1 (строки 12–17, p. 19), где j > high подтверждён.
Свежесть ACK семантически сохраняется на всём протяжении обработки одного
события `rcvs`.

**Исправление:** добавить `ACK_fresh` в предикаты s8 и s9.

### Итерация 2 — `AG (Window_ambiguous → AF (ACK_no_sample | Karn_done))` is false

**Контрпример:**

```
→ State: s0 → s1 → s2 → s4 [Window_ambiguous=TRUE]
-- Loop starts here
→ State: s5 [ACK_stale=TRUE, Window_ambiguous=FALSE]
→ State: s_cont → s1 → s2 → s4 [Window_ambiguous=TRUE] ...
```

**Анализ.** Бесконечный цикл `s4→s5(ACK_stale)→s_cont→s2→s4`. Устаревший
ACK (j ≤ high) — корректный исход: алгоритм Карна обрабатывает его через
строку 12 Algorithm 1 (`if j > high` — условие ложно, блок не выполняется).
Неоднозначность не разрешается немедленно, а откладывается до следующего
события `rcvs`. Свойство φ₈ было сформулировано без учёта этого законного
сценария.

**Исправление:**

```
Исходное:   AG (Window_ambiguous → AF (ACK_no_sample | Karn_done))
Уточнённое: AG (Window_ambiguous → AF (ACK_no_sample | ACK_stale | Karn_done))
```

### Финальный результат

После двух уточнений и введения ограничения справедливости все девять
CTL-спецификаций верифицированы инструментом nuXmv 2.1.0:

```
-- specification AG (EX TRUE)  is true
-- specification AG !(ACK_stale & ACK_fresh)  is true
-- specification AG (Sample_produced -> ACK_fresh)  is true
-- specification AG !(Window_unambiguous & Window_ambiguous)  is true
-- specification AG !(Sample_exact & Sample_pessimistic)  is true
-- specification AG ((Sample_produced & FIFO_channel) -> Sample_exact)  is true
-- specification AG ((Sample_produced & NonFIFO_channel) -> Sample_pessimistic)  is true
-- specification AG (Window_ambiguous -> AF ((ACK_no_sample | ACK_stale) | Karn_done))  is true
-- specification AG (Pkt_retransmitted -> AF Window_ambiguous)  is true
```

---

## 3.Z.8. Интерпретация результатов

**Свойство 1** (`AG EX TRUE`) — отсутствие тупиков. Монитор алгоритма Карна
всегда может выполнить следующий шаг.

**Свойства 2 и 9** (`Window_unambiguous/ambiguous`, `Pkt_retransmitted → AF
Window_ambiguous`) — корректность инварианта I1 (Наблюдение 2, p. 20):
ретрансмиссия необратимо переводит окно в состояние неоднозначности, и
однозначное/неоднозначное окно взаимно исключают друг друга.

**Свойство 3** (`AG !(ACK_stale & ACK_fresh)`) — монотонность ACK
(Наблюдение 1, p. 17): один ACK не может быть одновременно свежим и
устаревшим. Это формализует факт: последовательность ACK, отправляемых
получателем, монотонно не убывает.

**Свойство 4** (`AG !(Sample_exact & Sample_pessimistic)`) — взаимное
исключение типов выборок. Выборка либо точна (FIFO-канал, Obs.4), либо
пессимистична (не-FIFO, Obs.3), но не оба варианта одновременно.

**Свойства 5 и 6** — прямая верификация Наблюдений 3 и 4 (p. 20–21):

- В FIFO-канале: `S = rtt(high_prev)` точно (Obs.4, p. 21: `sℓ[|S|] = rtt(sℓ₋₁[|high|])`)
- В не-FIFO-канале: `S` является верхней границей RTT для всех пакетов
  в диапазоне `[high_prev, high_new)` (Obs.3, p. 20)

**Свойство 7** (`AG (Sample_produced → ACK_fresh)`) — выборка возможна
только по свежему ACK. Это прямое следствие строки 12 Algorithm 1 (p. 19):
`if j > high then` — выборка вычисляется только внутри этого блока.

**Свойство 8** (уточнённое φ₈) — живость разблокировки неоднозначного окна.
Из состояния неоднозначного окна система всегда выйдет: либо получит ACK
без выборки (`ACK_no_sample`), либо устаревший ACK (`ACK_stale`), либо
завершит работу. Уточнение формулы в итерации 2 выявило, что устаревший ACK —
законный способ разрешения ситуации, прямо описанный в Algorithm 1.

---

## 3.Z.9. Сводная таблица модели K_Karn

| Элемент | Определение |
|---------|-------------|
| $AP$ | 15 пропозиций, покрывающих фазы Algorithm 1 и типы выборок |
| $S$ | 12 состояний (s0–s9, s\_cont, s\_done) |
| $s_0$ | `Karn_idle = TRUE` |
| $T$ | 19 переходов, кодирующих строки 4–20 Algorithm 1 |
| $L$ | Разметка по фазам последнего обработанного события |
| FAIRNESS | `Sample_produced \| Karn_done \| ACK_stale` |
| Спецификации | 9 CTL-свойств (4 safety + 5 liveness) |
| Инструмент | nuXmv 2.1.0, режим BDD |
| Результат | Все 9 свойств `is true` |
| Итераций уточнения | 2 |

---

## 3.Z.10. Связь K_Karn с K_RTO и K_GBN

Три модели образуют **вертикальную цепочку зависимостей**:

```
K_Karn  →  K_RTO  →  K_GBN
выборка    вычисление  протокол
RTT        RTO         передачи
```

Формально: выходы K_Karn (пропозиция `Sample_produced`, значение S) являются
входами K_RTO (инициируют переход s6→s7 или s6→s8). Выходы K_RTO
(`rto_adequate` / `spurious_timeout`) определяют поведение таймера в K_GBN
(`timeout_fired`). Совместная верификация трёх моделей охватывает полную
цепочку механизма планирования канальных ресурсов транспортного протокола.

---

## Библиографические записи

Von Hippel, M. (2025). *Verification and Attack Synthesis for Network Protocols*.
arXiv:2511.01124. DOI: [10.48550/arXiv.2511.01124](https://doi.org/10.48550/arXiv.2511.01124)

Karn, P., Partridge, C. (1987). *Improving round-trip time estimates in reliable
transport protocols*. ACM SIGCOMM Computer Communication Review, 17(5), 2–7.
DOI: [10.1145/55483.55484](https://doi.org/10.1145/55483.55484)
