# Academic References & Methodology

This project, **EquiStabilize**, operationalizes advanced concepts from quantitative finance literature to address issues of market stability and liquidity resilience.

## 1. Market Making Strategy (Inventory Risk)

**Paper:** *High-frequency trading in a limit order book*  
**Authors:** Marco Avellaneda & Sasha Stoikov  
**Year:** 2008  
**Journal:** Quantitative Finance  
**Implementation in Project:** `src/strategies/MarketMaker.cpp`

### Summary
Avellaneda and Stoikov derived a closed-form solution for the optimal bid and ask quotes a market maker should post to maximize utility while managing inventory risk.

### Key Formula Implemented
The **Reservation Price** ($r$) is calculated as:
$$r(s, q, t) = s - q \gamma \sigma^2 (T - t)$$

Where:
- $s$: Current Mid Price
- $q$: Inventory Position (Positive = Long, Negative = Short)
- $\gamma$: Risk Aversion Parameter
- $\sigma$: Market Volatility

**Relevance to National Interest:**
By implementing this logic, the system demonstrates "Self-Stabilizing" behavior. Instead of withdrawing liquidity during stress (which causes Flash Crashes), the algorithm adjusts prices to naturally rebalance inventory, ensuring continuous market function.

---

## 2. Toxicity Detection (Flash Crash Warning)

**Paper:** *Flow Toxicity and Liquidity in a High-frequency World*  
**Authors:** David Easley, Marcos Lopez de Prado, & Maureen O'Hara  
**Year:** 2012  
**Journal:** The Review of Financial Studies  
**Implementation in Project:** `src/analytics/VPIN.cpp`

### Summary
The authors introduced **VPIN** (Volume-Synchronized Probability of Informed Trading), a metric designed to estimate the toxicity of the order flow. High VPIN values indicate a high probability of "Informed Trading" (predatory flow), which historically precedes liquidity crashes.

### Key Concept Implemented
- **Volume Bucketing:** Time is measured in "Volume" (e.g., every 10,000 shares) rather than seconds, adapting to market speed.
- **Order Imbalance:** Calculating the disparity between Buy and Sell volume within these buckets.

**Relevance to National Interest:**
This module acts as a "RegTech" (Regulatory Technology) tool. It provides an early warning signal for systemic instability, aligning with the goals of the SEC and CFTC to maintain fair and orderly markets.

---

## 3. Additional Concepts

**Fixed-Point Arithmetic**
- **Concept:** Using integer-based math for currency to avoid floating-point errors (IEEE 754).
- **Implementation:** `inc/Order.h`
- **Relevance:** Critical for the accuracy and auditability of financial infrastructure.
