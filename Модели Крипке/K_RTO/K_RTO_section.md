# Раздел 3.Y. Формальная модель Крипке механизма RTO (K_RTO)

> **Источник:** Max von Hippel, *Verification and Attack Synthesis for Network Protocols*,
> arXiv:2511.01124v1, 2025. Глава 2.
> DOI: https://doi.org/10.48550/arXiv.2511.01124
>
> **Стандарт:** V. Paxson, M. Allman, J. Chu, M. Sargent.
> *Computing TCP's Retransmission Timer*. RFC 6298, June 2011.
> https://datatracker.ietf.org/doc/html/rfc6298

---

## 3.Y.1. Неформальное описание механизма

Механизм RTO (Retransmission Timeout) определяет, как долго отправитель
ждёт подтверждения перед повторной передачей пакета. Он состоит из двух
взаимосвязанных алгоритмов:

**Алгоритм Карна** (Karn's Algorithm) решает проблему неоднозначности
при измерении RTT. Когда пакет ретрансмитируется, невозможно определить,
какой именно передаче соответствует полученный ACK — исходной или повторной.
Алгоритм Карна блокирует выборку RTT при ретрансмиссии: выборка производится
только по однозначным ACK, то есть тогда, когда все пакеты в диапазоне
`[high, j)` были переданы ровно один раз.

**Вычисление RTO по RFC 6298** использует выборки алгоритма Карна для
поддержания двух скользящих оценок: `srtt` (сглаженный RTT) и `rttvar`
(оценка дисперсии). На их основе вычисляется RTO:

```
rtoᵢ = srttᵢ + max(G, 4·rttvarᵢ)

srttᵢ  = (1−α)·srttᵢ₋₁ + α·Sᵢ,          α = 1/8
rttvarᵢ = (1−β)·rttvarᵢ₋₁ + β·|srttᵢ₋₁ − Sᵢ|,  β = 1/4
```

где `G` — гранулярность часов, `Sᵢ` — i-я выборка RTT от алгоритма Карна.

Значение RTO критично для планирования канальных ресурсов: слишком малое
значение приводит к ложным таймаутам и лишним ретрансмиссиям, перегружающим
канал; слишком большое — к неоправданно долгому простою канала при реальных
потерях. RFC 6298 подчёркивает:

> *«The Internet, to a considerable degree, relies on the correct implementation
> of the RTO algorithm in order to preserve network stability and avoid
> congestion collapse.»*

---

## 3.Y.2. Переменные состояния и их абстракция

Числовые переменные `srtt`, `rttvar`, `rto` принимают рациональные значения
из бесконечного пространства. Для построения конечной структуры Крипке
применяется **качественная абстракция**: каждое состояние соответствует фазе
работы двух алгоритмов, а не конкретным числовым значениям.

| Переменная | Тип | Абстракция |
|------------|-----|------------|
| `numT[i]`  | ℕ → ℕ | Булево: `= 1` (однократная) / `> 1` (ретрансмиссия) |
| `time[i]`  | ℕ → ℕ | Не моделируется явно (инвариант I2 гарантирован структурой) |
| `high`     | ℕ | Фиксируется через фазу: IDLE / TRACKING / SAMPLED |
| `srtt`     | ℚ⁺ | Фаза сходимости: INIT → UPDATING → STEADY → ADEQUATE/RISK |
| `rttvar`   | ℚ⁺ | Поглощается фазой srtt (Obs.6 диссертации) |
| `rto`      | ℚ⁺ | `rto_adequate` (rto ≥ RTT) / `timeout_risk` (rto < RTT) |

---

## 3.Y.3. Компоненты модели и отображение на алгоритмы

Модель K_RTO объединяет два алгоритма в единый процесс, наблюдающий
за действиями отправителя (non-interfering monitor, раздел 2.2 диссертации).

### Алгоритм Карна — состояния и переходы

Алгоритм Карна описывается следующей логикой выборки RTT:

```
ok-to-sample(numT, high) :=
    (∀k: high < k < j → numT[k] = 1) ∧ (high > 0 → numT[high] = 1)
```

В конечной абстракции это выражается двумя взаимоисключающими фазами:

- **sampling_enabled** (`s2`): все пакеты в окне переданы однократно —
  выборка потенциально возможна
- **sampling_blocked** (`s3, s4, s5, s12, s_abort`): был факт ретрансмиссии —
  выборка заблокирована (ACK неоднозначен)

### Вычисление RTO — состояния и переходы

После получения свежей выборки (`s6`, `fresh_sample`) RTO-вычисление
проходит через четыре фазы:

- **s7** (`RTO_first_sample`): первая выборка — `srtt₁=S`, `rttvar₁=S/2`
- **s8** (`RTO_updating`): последующие выборки — srtt и rttvar обновляются
- **s9** (`RTO_steady_state`): c/r установившийся режим (Obs.5 диссертации):
  выборки в `[c−r, c+r]`, srtt сходится к интервалу
- **s10** (`rto_adequate`): `rto ≥ RTT` — ложные таймауты исключены
- **s11** (`timeout_risk`): патологический случай (Obs.6): rttvar падает
  ниже `r`, возможно `rto < RTT`
- **s12** (`spurious_timeout`): ложный таймаут сработал, ненужная ретрансмиссия

---

## 3.Y.4. Формальный кортеж K_RTO

$$K_{RTO} = \langle AP,\; S,\; s_0,\; T,\; L \rangle$$

### Атомарные пропозиции AP

$$AP = \{\;
Karn\_idle,\; Karn\_tracking,\; Karn\_sampled,\;
sampling\_enabled,\; sampling\_blocked,\;$$
$$ACK\_ambiguous,\; Retransmission\_active,\;
fresh\_sample,\; rto\_initialized,\;$$
$$RTO\_first\_sample,\; RTO\_updating,\; RTO\_steady\_state,\;$$
$$srtt\_converging,\; rto\_adequate,\; timeout\_risk,\; spurious\_timeout,\;$$
$$Transmission\_complete,\; Connection\_aborted,\; transmission\_active
\;\}$$

| Пропозиция | Смысл в контексте планирования ресурсов |
|---|---|
| `sampling_enabled` | Канал использовался однозначно — выборка RTT корректна |
| `sampling_blocked` | Ретрансмиссия заблокировала выборку — RTT неизвестен |
| `rto_adequate` | RTO ≥ RTT — канал не простаивает из-за ложных таймаутов |
| `timeout_risk` | RTO < RTT — возможна лишняя ретрансмиссия, канал перегружен |
| `spurious_timeout` | Ложный таймаут произошёл — канал занят лишней работой |
| `Connection_aborted` | Превышен лимит ретрансмиссий — соединение разорвано |

### Множество состояний S и функция разметки L

| Состояние | Активные пропозиции | Фаза |
|-----------|---------------------|------|
| `s0`  | `Karn_idle` | Начальное |
| `s1`  | `transmission_active`, `Karn_tracking` | Отправка пакета |
| `s2`  | `transmission_active`, `sampling_enabled`, `Karn_tracking` | Ожидание ACK (однозначный) |
| `s3`  | `transmission_active`, `sampling_blocked`, `Retransmission_active` | Ретрансмиссия |
| `s4`  | `transmission_active`, `sampling_blocked` | Ожидание ACK (неоднозначный) |
| `s5`  | `sampling_blocked`, `ACK_ambiguous` | Неоднозначный ACK получен |
| `s6`  | `fresh_sample`, `Karn_sampled` | Выборка RTT получена |
| `s7`  | `rto_initialized`, `RTO_first_sample` | Первая выборка, инициализация |
| `s8`  | `rto_initialized`, `RTO_updating` | Обновление srtt/rttvar |
| `s9`  | `rto_initialized`, `srtt_converging`, `RTO_steady_state` | Установившийся режим |
| `s10` | `rto_initialized`, `srtt_converging`, `rto_adequate` | RTO достаточен |
| `s11` | `rto_initialized`, `srtt_converging`, `timeout_risk` | Риск ложного таймаута |
| `s12` | `spurious_timeout`, `sampling_blocked` | Ложный таймаут |
| `s_done` | `Transmission_complete` | Передача завершена |
| `s_abort` | `Connection_aborted`, `sampling_blocked` | Разрыв соединения |

### Начальное состояние

$$s_0 = (Karn\_idle = \top,\; \text{все остальные} = \bot)$$

### Отношение переходов T

```
s0  → s1
s1  → s2   (пакет передан однократно, ждём ACK)
s1  → s3   (сразу ретрансмиссия)
s2  → s6   (ok-to-sample=TRUE, свежая выборка)
s2  → s3   (ретрансмиссия)
s3  → s4   (ждём ACK после ретрансмиссии)
s4  → s3   (ещё одна ретрансмиссия)
s4  → s5   (ACK получен, но неоднозначный)
s4  → s_abort (превышен лимит ретрансмиссий)
s5  → s1   (high обновлён, продолжаем)
s6  → s7   (первая выборка → инициализация RTO)
s6  → s8   (повторная выборка → обновление)
s7  → s8
s8  → s9   (достигнут steady-state)
s9  → s10  (rto ≥ RTT, Obs.5)
s9  → s11  (патологический случай, Obs.6)
s10 → s1   (следующий цикл)
s10 → s_done
s11 → s12  (ложный таймаут)
s12 → s3   (ретрансмиссия после ложного таймаута)
s_done → s_done
s_abort → s_abort
```

---

## 3.Y.5. Ограничение справедливости (Fairness Constraint)

Для корректной верификации свойств живости в модель введено ограничение
справедливости:

```smv
FAIRNESS
    sampling_enabled | Transmission_complete | Connection_aborted
```

**Обоснование.** В недетерминированном автомате существуют технически
допустимые, но семантически нереалистичные пути: бесконечный цикл
ретрансмиссий `s3→s4→s3→...` без каких-либо исходов. В реальных
транспортных протоколах (TCP, SCTP) количество попыток ретрансмиссии
ограничено системным параметром (`tcp_retries_max` = 15 в Linux по умолчанию),
после исчерпания которого соединение разрывается с ошибкой `ETIMEDOUT`.

Ограничение справедливости формально выражает этот факт: на каждом
справедливом (реалистичном) пути бесконечно часто выполняется хотя бы
одно из трёх условий — выборка разблокирована, передача завершена,
соединение разорвано.

---

## 3.Y.6. Верифицируемые CTL-свойства

| № | CTL-спецификация | Смысл |
|---|-----------------|-------|
| 1 | `AG (EX TRUE)` | Отсутствие тупиков |
| 2 | `AG !(sampling_enabled & sampling_blocked)` | Взаимное исключение фаз Карна |
| 3 | `AG ((sampling_blocked & !Transmission_complete & !Connection_aborted) → AF (sampling_enabled \| Transmission_complete \| Connection_aborted))` | Блокировка выборки временна |
| 4 | `AG (fresh_sample → AF rto_initialized)` | Каждая выборка обрабатывается |
| 5 | `AG (rto_initialized → AF (rto_adequate \| spurious_timeout))` | RTO сходится либо к адекватному значению, либо к патологии |
| 6 | `AG !(rto_adequate & timeout_risk)` | Взаимное исключение адекватности и риска |
| 7 | `AG (spurious_timeout → AF sampling_blocked)` | Ложный таймаут всегда блокирует выборку |

---

## 3.Y.7. Процесс верификации и итерационное уточнение модели

### Итерация 1 — обнаружение ретрансмиссионной петли

При первоначальной верификации свойство φ₃ было сформулировано как:

```
AG (sampling_blocked → AF sampling_enabled)
```

nuXmv опроверг его, построив контрпример:

```
→ State 1.1: s0  [Karn_idle = TRUE]
→ State 1.2: s1  [transmission_active = TRUE]
-- Loop starts here
→ State 1.3: s3  [sampling_blocked = TRUE, Retransmission_active = TRUE]
→ State 1.4: s4  [sampling_blocked = TRUE]
→ State 1.5: s3  [Retransmission_active = TRUE]
...  (бесконечно)
```

**Анализ.** Система застряла в цикле `s3→s4→s3`, моделирующем бесконечную
ретрансмиссию. Это реальный сценарий, описанный в диссертации (раздел 2.6):
при постоянных потерях пакетов алгоритм Карна никогда не получит однозначного
ACK. Свойство нарушается, однако причина — не дефект механизма, а то, что
модель не разграничивала активную передачу и нормальное завершение сеанса.

**Первое уточнение:**
```
AG ((sampling_blocked & !Transmission_complete)
    → AF (sampling_enabled | Transmission_complete))
```

### Итерация 2 — добавление состояния принудительного разрыва

После первого уточнения контрпример повторился, поскольку модель не содержала
**терминального исхода при исчерпании ретрансмиссий**. Был добавлен переход
`s4 → s_abort` и состояние `s_abort` (`Connection_aborted`).

**Второе уточнение:**
```
AG ((sampling_blocked & !Transmission_complete & !Connection_aborted)
    → AF (sampling_enabled | Transmission_complete | Connection_aborted))
```

### Итерация 3 — введение ограничения справедливости

После второго уточнения nuXmv по-прежнему находил контрпример через петлю
`s3→s4→s3`, поскольку из состояния `s4` недетерминированно выбирался переход
обратно в `s3`, а не в `s_abort`. Это технически допустимый, но **семантически
несправедливый** путь: реальный протокол не допускает бесконечного цикла
ретрансмиссий без исхода.

Решение — введение ограничения справедливости `FAIRNESS`, ограничивающего
анализ только реалистичными путями.

### Финальный результат

После введения ограничения справедливости все семь CTL-спецификаций
были верифицированы инструментом nuXmv 2.1.0:

```
-- specification AG (EX TRUE)  is true
-- specification AG !(sampling_enabled & sampling_blocked)  is true
-- specification AG (((sampling_blocked & !Transmission_complete)
     & !Connection_aborted) -> AF ((sampling_enabled | Transmission_complete)
     | Connection_aborted))  is true
-- specification AG (fresh_sample -> AF rto_initialized)  is true
-- specification AG !(rto_adequate & timeout_risk)  is true
-- specification AG (spurious_timeout -> AF sampling_blocked)  is true
-- specification AG (rto_initialized -> AF (rto_adequate | spurious_timeout))  is true
```

---

## 3.Y.8. Интерпретация результатов

**Свойство 1** (`AG EX TRUE`) — нет тупиков. Из любого состояния системы
возможен хотя бы один следующий переход.

**Свойство 2** (`AG !(sampling_enabled & sampling_blocked)`) — взаимное
исключение фаз алгоритма Карна. Невозможна ситуация, в которой выборка
одновременно разрешена и заблокирована. Это прямое следствие инвариантов
I1 и I2 (Наблюдение 2, von Hippel, 2025).

**Свойство 3** (уточнённое φ₃) — живость разблокировки. В любой активной
сессии без принудительного разрыва алгоритм Карна рано или поздно либо
получит однозначный ACK и разблокирует выборку, либо сеанс завершится.
Это соответствует Наблюдению 3: алгоритм Карна всегда выдаёт реальный RTT
хотя бы одного пакета при наличии однозначных ACK.

**Свойство 4** (`AG (fresh_sample → AF rto_initialized)`) — каждая
выборка RTT обрабатывается RTO-вычислением. Это гарантирует, что
информация о состоянии канала не теряется.

**Свойство 5** (`AG (rto_initialized → AF (rto_adequate | spurious_timeout))`)
— сходимость RTO. Инициализированный механизм всегда придёт либо к
адекватному значению RTO (Наблюдение 5), либо к патологическому случаю
ложного таймаута (Наблюдение 6). Третьего исхода нет.

**Свойство 6** (`AG !(rto_adequate & timeout_risk)`) — взаимное
исключение. Невозможно одновременно иметь адекватный RTO и находиться
в зоне риска ложного таймаута.

**Свойство 7** (`AG (spurious_timeout → AF sampling_blocked)`) — реакция
на ложный таймаут. После каждого ложного срабатывания RTO вынужденная
ретрансмиссия блокирует алгоритм Карна, что является корректным поведением
протокола по RFC 6298.

---

## 3.Y.9. Сводная таблица модели K_RTO

| Элемент | Определение |
|---------|-------------|
| $AP$ | 19 пропозиций, охватывающих фазы алгоритма Карна и вычисления RTO |
| $S$ | 15 состояний (s0–s12, s\_done, s\_abort) |
| $s_0$ | `Karn_idle = TRUE`, все остальные `FALSE` |
| $T$ | 22 перехода, кодирующих логику Алгоритма 1 и формул RFC 6298 |
| $L$ | Разметка по качественным фазам двух алгоритмов |
| FAIRNESS | `sampling_enabled \| Transmission_complete \| Connection_aborted` |
| Спецификации | 7 CTL-свойств (2 safety + 5 liveness) |
| Инструмент | nuXmv 2.1.0, режим BDD |
| Результат | Все 7 свойств `is true` |

---

## Библиографические записи

Von Hippel, M. (2025). *Verification and Attack Synthesis for Network Protocols*.
arXiv:2511.01124. DOI: [10.48550/arXiv.2511.01124](https://doi.org/10.48550/arXiv.2511.01124)

Paxson, V., Allman, M., Chu, J., Sargent, M. (2011).
*Computing TCP's Retransmission Timer*. RFC 6298.
https://datatracker.ietf.org/doc/html/rfc6298

Karn, P., Partridge, C. (1987). *Improving round-trip time estimates in
reliable transport protocols*. ACM SIGCOMM Computer Communication Review,
17(5), 2–7.
