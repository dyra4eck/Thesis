# Справочник формул: точные страницы в диссертации

**Источник:** Max von Hippel,
*Verification and Attack Synthesis for Network Protocols*,
arXiv:2511.01124v1, 2025.
DOI: https://doi.org/10.48550/arXiv.2511.01124

---

## Глава 2 — Алгоритм Карна и вычисление RTO

### Алгоритм Карна (Algorithm 1) — стр. 19

```
Вход:  snds(i), rcvs(j),  i, j ∈ ℕ⁺
Выход: S ∈ ℕ⁺

numT[i] := numT[i] + 1        -- стр. 19 (line 5 алгоритма)
time[i] := τ                   -- стр. 19 (line 7, первая передача)
S := τ − time[high]            -- стр. 19 (line 14, вычисление выборки)
```

### Условие ok-to-sample — стр. 20

```
ok-to-sample(numT, high) :=
    (∀k: high < k < j → numT[k] = 1)
    ∧ (high > 0 → numT[high] = 1)
```

> Источник: von Hippel (2025), p. 20 (раздел 2.3, после Algorithm 1)
> Смысл: выборка RTT разрешена только если все пакеты в диапазоне
> [high, j) переданы ровно один раз.

### Формулы RFC 6298 — стр. 22

```
rtoᵢ  = srttᵢ + max(G, 4·rttvarᵢ)

srttᵢ  = Sᵢ                               если i = 1
srttᵢ  = (1 − α)·srttᵢ₋₁ + α·Sᵢ          если i > 1

rttvarᵢ = Sᵢ/2                             если i = 1
rttvarᵢ = (1 − β)·rttvarᵢ₋₁ + β·|srttᵢ₋₁ − Sᵢ|  если i > 1
```

> Источник: von Hippel (2025), p. 22 (раздел 2.5);
> оригинальный стандарт: RFC 6298, §2 (Paxson et al., 2011)
> Параметры: α = 1/8, β = 1/4, G — гранулярность часов

### Наблюдение 5 (сходимость srtt) — стр. 22

```
L = (1−α)^(n+1) · srttᵢ₋₁ + (1 − (1−α)^(n+1)) · (c−r)
H = (1−α)^(n+1) · srttᵢ₋₁ + (1 − (1−α)^(n+1)) · (c+r)

L ≤ srttᵢ₊ₙ ≤ H

lim(n→∞) L = c − r
lim(n→∞) H = c + r
```

> Источник: von Hippel (2025), p. 22 (Observation 5, раздел 2.6)
> Условие: Sᵢ, …, Sᵢ₊ₙ — c/r установившиеся выборки (в интервале [c−r, c+r])

### Наблюдение 6 (ограничение rttvar) — стр. 22–23

```
B_Δ(x) = (1−β)·x + β·Δ

rttvarⱼ ≤ B_Δ(rttvarⱼ₋₁)

B_Δ^(n+1)(rttvarᵢ₋₁) = (1−β)^(n+1)·rttvarᵢ₋₁ + (1 − (1−β)^(n+1))·Δ

lim(n→∞) B_Δ^(n+1)(rttvarᵢ₋₁) = Δ
```

> Источник: von Hippel (2025), p. 22–23 (Observation 6, раздел 2.6)
> Условие: |Sⱼ − srttⱼ₋₁| ≤ Δ для всех j ∈ [i, i+n]

---

## Глава 3 — Формальная модель Go-Back-N

### Отношение переходов senderR — стр. 39, формула (3.1)

```
senderR(s, e, s') :=
  (∃a ∈ ℕ⁺ :: e = rcvs(a, ACK) ∧ (s', ⊥) = rcvAck(s, e))
  ∨
  (∃x ∈ Str :: e = snds(curPkt, x) ∧ curPkt < hiAck+N
               ∧ (s', e) = advCur(s, x))
  ∨
  (e = ⊥ ∧ curPkt = hiAck+N ∧ (s', e) = timeout(s))
```

> Источник: von Hippel (2025), p. 39, формула (3.1), раздел 3.3

### Инварианты отправителя (Теорема 1) — стр. 39–40

```
Inv1: hiAck ≤ hiPkt + 1
Inv2: hiAck ≤ curPkt ≤ hiAck + N
Inv3: hiAck и hiPkt монотонно не убывают
```

> Источник: von Hippel (2025), p. 39–40, Theorem 1, раздел 3.3

### Отношение переходов receiverR — стр. 40, формула (3.2)

```
receiverR(r, e, r') :=
  (∃i ∈ ℕ⁺ :: e = sndr(i, ACK) ∧ (r', e) = sndAck(r))
  ∨
  (∃x ∈ Str, i ∈ ℕ⁺ :: e = rcvr(i, x) ∧ (r', ⊥) = rcvPkt(r, e))
```

> Источник: von Hippel (2025), p. 40, формула (3.2), раздел 3.4

### Подотношения TBF — стр. 42, формулы (3.3)–(3.6)

```
tbfIntR(tbf, e, tbf') :=                              -- формула (3.3)
  e = ⊥ ∧ (tbf', e) ∈ {tick(tbf), decay(tbf)}
  ∨ ∃i ∈ ℕ :: i < len(dgs) ∧ (tbf', e) = drop(tbf, i)

tbfProcR(tbf, e, tbf') :=                             -- формула (3.4)
  ∃d ∈ Dg :: e = snda(d) ∧ tbf' = process(tbf, e)

tbfFwdR(tbf, e, tbf') :=                              -- формула (3.5)
  ∃i ∈ ℕ :: i < len(dgs) ∧ sz(dgs[i]) ≤ bkt
  ∧ e = rcvb(dgs[i]) ∧ tbf' = forward(tbf, i)

tbfR := tbfIntR ∨ tbfProcR ∨ tbfFwdR                 -- формула (3.6)
```

> Источник: von Hippel (2025), p. 42–43, формулы (3.3)–(3.6), раздел 3.5

### Составное отношение переходов sysR — стр. 45–46, формула (3.11)

```
sysR(sys, e, sys') :=
  senderSnd(sys, e, sys')    -- snds(d): Sender + TBF_s синхронизируются
  ∨ receiverSnd(...)         -- sndr(d): Receiver + TBF_r
  ∨ senderInt(...)           -- timeout: внутреннее событие Sender
  ∨ tbfSint(...)             -- tick/decay/drop в TBF_s
  ∨ tbfRint(...)             -- tick/decay/drop в TBF_r
  ∨ senderRcv(...)           -- rcvs(d): Sender + TBF_r
  ∨ receiverRcv(...)         -- rcvr(d): Receiver + TBF_s
```

> Источник: von Hippel (2025), p. 45–46, формула (3.11), раздел 3.6
> Подформулы (3.7)–(3.10) — p. 45–46

### Эффективность GB(N) (Теорема 6) — стр. 49

```
efficiency = (R·(dcap_s − R)/(R − rt_s) + R + rt_s) / N
```

> Источник: von Hippel (2025), p. 49, Theorem 6, раздел 3.7
> Условие: sender постоянно перегружает канал (R > rt_s)

---

## Глава 4 — Модели Крипке и LTL

### Definition 1 — конечная структура Крипке — стр. 62

```
K = ⟨AP, S, s₀, T, L⟩

AP — конечное множество атомарных пропозиций
S  — конечное множество состояний
s₀ ∈ S — начальное состояние
T ⊆ S × S — отношение переходов
L : S → 2^AP — функция разметки (тотальная)
```

> Источник: von Hippel (2025), p. 62, Definition 1, раздел 4.3

### Definition 2 — Процесс — стр. 63

```
P = ⟨AP, I, O, S, s₀, T, L⟩

T ⊆ S × (I ∪ O) × S
I ∩ O = ∅
```

> Источник: von Hippel (2025), p. 63, Definition 2, раздел 4.4
> Отличие от K: переходы маркированы событиями из I ∪ O

### Definition 3 — параллельная композиция, формула (4.3) — стр. 64–65

```
P₁ ∥ P₂ = ⟨AP₁ ∪ AP₂,
             (I₁ ∪ I₂) \ (O₁ ∪ O₂),
             O₁ ∪ O₂,
             S₁ × S₂,
             (s₀¹, s₀²),
             T, L⟩

L(s₁, s₂) = L₁(s₁) ∪ L₂(s₂)
```

> Источник: von Hippel (2025), p. 64, Definition 3, формула (4.3), раздел 4.4
> Требования: O₁ ∩ O₂ = ∅, AP₁ ∩ AP₂ = ∅

### Семантика LTL, формула (4.2) — стр. 63

```
σ |= p           ⟺  p ∈ σ[0]
σ |= ϕ₁ ∧ ϕ₂    ⟺  σ |= ϕ₁ и σ |= ϕ₂
σ |= ¬ϕ₁        ⟺  σ ⊭ ϕ₁
σ |= X ϕ₁        ⟺  σ[1:] |= ϕ₁
σ |= ϕ₁ U ϕ₂    ⟺  ∃κ ≥ 0: σ[κ:] |= ϕ₂ ∧ ∀0 ≤ j < κ: σ[j:] |= ϕ₁
```

> Источник: von Hippel (2025), p. 63, формула (4.2), раздел 4.3

---

## Краткая справка: что откуда

| Формула | Страница | Раздел |
|---------|----------|--------|
| Algorithm 1 (Karn) — псевдокод | p. 19 | §2.3 |
| ok-to-sample | p. 20 | §2.3 |
| rto, srtt, rttvar (RFC 6298) | p. 21 | §2.5 |
| Obs.5: bounds on srtt | p. 22 | §2.6 |
| Obs.6: bound on rttvar | p. 22–23 | §2.6 |
| senderR (3.1) | p. 39 | §3.3 |
| Inv1–Inv3 (Thm.1) | p. 39–40 | §3.3 |
| receiverR (3.2) | p. 40 | §3.4 |
| tbfIntR (3.3) | p. 42 | §3.5 |
| tbfProcR (3.4) | p. 42 | §3.5 |
| tbfFwdR (3.5) | p. 42 | §3.5 |
| tbfR (3.6) | p. 43 | §3.5 |
| senderSnd (3.7) | p. 45 | §3.6 |
| sysR (3.11) | p. 46 | §3.6 |
| Efficiency formula (Thm.6) | p. 49 | §3.7 |
| Definition 1 (Kripke) | p. 62 | §4.3 |
| LTL semantics (4.2) | p. 63 | §4.3 |
| Definition 2 (Process) | p. 63 | §4.4 |
| Definition 3, formula (4.3) | p. 64 | §4.4 |
