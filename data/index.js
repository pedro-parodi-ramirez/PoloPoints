const STATUS = {
    OK: 200,
    ACCEPTED: 202,
    BAD_REQUEST: 400,
    NOT_FOUND: 404,
    INTERNAL_SERVER_ERROR: 500
};
const timerStatus = {
    STOPPED: 0,
    RUNNING: 1
}
const gameStatus = {
    IN_PROGRESS: 0,
    EXTENDED_TIME: 1,
    HALFTIME: 2
}
const REQUEST_PERIOD = 250;

const btnUpVisitor = document.getElementById('up-visitor');
const btnDownVisitor = document.getElementById('down-visitor');
const btnUpLocal = document.getElementById('up-local');
const btnDownLocal = document.getElementById('down-local');
const btnUpChukker = document.getElementById('up-chukker');
const btnDownChukker = document.getElementById('down-chukker');
const btnStartTimer = document.getElementById('start-timer');
const btnStopTimer = document.getElementById('stop-timer');
const btnResetTimer = document.getElementById('reset-timer');
const btnResetAll = document.getElementById('reset-all');
const btnSetDefaultTimer = document.getElementById('set-default-timer');
const btnUpMinute = document.getElementById('up-minute');
const btnDownMinute = document.getElementById('down-minute');
const btnUpSecond = document.getElementById('up-second');
const btnDownSecond = document.getElementById('down-second');
const IP = "192.168.1.5";
let refreshTimer = null;
const localValue = document.getElementById('local');
const visitorValue = document.getElementById('visitor');
const chukkerValue = document.getElementById('chukker');
const timerMMValue = document.getElementById('timer-mm');
const timerSSValue = document.getElementById('timer-ss');
const intervalMark = document.getElementById('interval-mark');
let timerState = timerStatus.STOPPED;
let gameState = gameStatus.IN_PROGRESS;

const command = {
    INC_SCORE_LOCAL: 1,
    INC_SCORE_VISITOR: 2,
    DEC_SCORE_LOCAL: 3,
    DEC_SCORE_VISITOR: 4,
    INC_CHUKKER: 5,
    DEC_CHUKKER: 6,
    START_TIMER: 7,
    STOP_TIMER: 8,
    RESET_TIMER: 9,
    SET_CURRENT_TIMER: 10,
    SET_DEFAULT_TIMER: 11,
    SET_HALFTIME_TIMER: 12,
    RESET_ALL: 13
};

const dataIndex = {
    LOCAL: 0,
    VISITOR: 1,
    CHUKKER: 2,
    TIMER_MM: 3,
    TIMER_SS: 4,
    TIMER_STATE: 5,
    GAME_STATE: 6
}

/* -------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------- EVENTS ---------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */

// Visitor
btnUpVisitor.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_VISITOR}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownVisitor.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_VISITOR}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// Local
btnUpLocal.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_LOCAL}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownLocal.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_LOCAL}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// chukker
btnUpChukker.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/chukker?cmd=${command.INC_CHUKKER}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownChukker.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/chukker?cmd=${command.DEC_CHUKKER}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// Timer
btnStartTimer.addEventListener('click', async () => {
    if (parseInt(timerMMValue.value) === 0 && parseInt(timerSSValue.value) === 0) {
        alert('Timer cannot be 00:00.');
        return;
    }
    else {
        const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.START_TIMER}`);

        if (rawResponse.status === STATUS.ACCEPTED) {
            if (refreshTimer === null) { startAutoRequest(); }
        }
        else { alert('Something went wrong.'); }
    }
});

btnStopTimer.addEventListener('click', async () => {
    if (timerState === timerStatus.RUNNING) {
        const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.STOP_TIMER}`);
        if (rawResponse.status !== STATUS.ACCEPTED) { alert('Something went wrong.'); }
    }
});

btnUpMinute.addEventListener('click', async () => {
    let mmValue = parseInt(timerMMValue.value) + 1;
    let ssValue = parseInt(timerSSValue.value);
    sendTimerData(mmValue, ssValue, command.SET_CURRENT_TIMER);
});

btnDownMinute.addEventListener('click', async () => {
    let mmValue = parseInt(timerMMValue.value) - 1;
    let ssValue = parseInt(timerSSValue.value);
    sendTimerData(mmValue, ssValue, command.SET_CURRENT_TIMER);
});

btnUpSecond.addEventListener('click', async () => {
    let mmValue = parseInt(timerMMValue.value);
    let ssValue = parseInt(timerSSValue.value) + 1;
    sendTimerData(mmValue, ssValue, command.SET_CURRENT_TIMER);
});

btnDownSecond.addEventListener('click', async () => {
    let mmValue = parseInt(timerMMValue.value);
    let ssValue = parseInt(timerSSValue.value) - 1;
    sendTimerData(mmValue, ssValue, command.SET_CURRENT_TIMER);
});

btnSetDefaultTimer.addEventListener('click', async () => {
    let mmValue = parseInt(timerMMValue.value);
    let ssValue = parseInt(timerSSValue.value);
    if (mmValue === 0 && ssValue === 0) { alert('Timer cannot be 00:00.'); }
    else { sendTimerData(mmValue, ssValue, command.SET_DEFAULT_TIMER); }
});

btnResetTimer.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.RESET_TIMER}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        setOptions();
        stopAutoRequest();
    }
    else { alert('Something went wrong.'); }
});

// Reset all
btnResetAll.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/reset`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        setOptions();
        stopAutoRequest();
    }
    else { alert('Something went wrong.'); }
});

/* -------------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------- MAIN ----------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */

// Al cargarse el sitio web, buscar datos del tablero.
window.addEventListener('load', async () => {
    await refreshScoreboard();
    if (timerState === timerStatus.RUNNING) {
        if (refreshTimer === null) { startAutoRequest(); }
    }
});

// Solicitar datos y refrescar el tablero
async function refreshScoreboard() {
    const rawResponse = await fetch(`http://${IP}/scoreboard`);
    if (rawResponse.status === STATUS.OK) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        setOptions();
        if (timerState === timerStatus.STOPPED) { stopAutoRequest(); }
    }
}

// Se configura un periodo de XX ms en los cuales se consulta el estado del tablero
function startAutoRequest() {
    refreshTimer = setInterval(async () => {
        await refreshScoreboard();
    }, REQUEST_PERIOD);
}

// Detener el autorequest de datos de tablero
function stopAutoRequest() {
    clearInterval(refreshTimer);
    refreshTimer = null;
}

// Plasmar datos en tablero
function setScoreboardValues(dataString) {
    const data = dataString.split(',');
    localValue.value = data[dataIndex.LOCAL].padStart(2, '0');
    visitorValue.value = data[dataIndex.VISITOR].padStart(2, '0');
    chukkerValue.value = data[dataIndex.CHUKKER];
    timerMMValue.value = data[dataIndex.TIMER_MM].padStart(2, '0');
    timerSSValue.value = data[dataIndex.TIMER_SS].padStart(2, '0');
    timerState = parseInt(data[dataIndex.TIMER_STATE]);
    gameState = parseInt(data[dataIndex.GAME_STATE]);
}

// Enviar valores de timer a servidor
async function sendTimerData(mm, ss, cmd) {
    const rawResponse = await fetch(`http://${IP}/timer/set?mm=${mm}&ss=${ss}&cmd=${cmd}}`);
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        setOptions();
        if (cmd === command.SET_DEFAULT_TIMER) { alert('Timer updated.'); }
    }
}

// Fijar opciones en front-end segun estado de timer
function setOptions() {
    if (gameState != gameStatus.HALFTIME) { intervalMark.hidden = true; }
    else { intervalMark.hidden = false; }
    if (timerState === timerStatus.STOPPED) {
        btnStopTimer.disabled = true;
        btnStartTimer.disabled = false;
        btnUpMinute.disabled = false;
        btnDownMinute.disabled = false;
        btnUpSecond.disabled = false;
        btnDownSecond.disabled = false;
        btnResetTimer.disabled = false;
        if (gameState === gameStatus.IN_PROGRESS) { btnSetDefaultTimer.disabled = false; }
    }
    else if (timerState === timerStatus.RUNNING) {
        btnStopTimer.disabled = false;
        btnStartTimer.disabled = true;
        btnUpMinute.disabled = true;
        btnDownMinute.disabled = true;
        btnUpSecond.disabled = true;
        btnDownSecond.disabled = true;
        btnResetTimer.disabled = true;
        btnSetDefaultTimer.disabled = true;
    }
    else {
        btnStopTimer.disabled = true;
        btnStartTimer.disabled = true;
        btnUpMinute.disabled = true;
        btnDownMinute.disabled = true;
        btnUpSecond.disabled = true;
        btnDownSecond.disabled = true;
        btnResetTimer.disabled = false;
        btnSetDefaultTimer.disabled = true;
    }
}