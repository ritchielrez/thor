$$
\begin{align}
    \text{[prog]} &\to \text{[stmt]}^* \\
    \text{[stmt]} &\to
    \begin{cases}
        \text{id} :: \text{fn} \text{[scope]}\\
        \text{exit}(\text{[exp]}); \\
        \text{[scope]} \\
    \end{cases} \\
    \text{[scope]} &\to \{\text{[stmt]}^*\} \\
\end{align}
$$
