import { useMemo } from "react";
import {
  type EventBinding,
  type EventCriteriaKind,
  type EventEntityKey,
  type EventTypeInfo,
  eventTypeByTrigger,
  getEventTypesForEntity
} from "../data/eventTypes";

type EventBindingsViewProps = {
  entityType: EventEntityKey | null;
  events: EventBinding[];
  script: string;
  canEdit: boolean;
  onChange: (events: EventBinding[]) => void;
};

const callbackRegex =
  /^\s*(?:fun\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\([^)]*\)\s*\{/gm;

function extractCallbacks(script: string): string[] {
  const callbacks = new Set<string>();
  callbackRegex.lastIndex = 0;
  let match: RegExpExecArray | null;
  while ((match = callbackRegex.exec(script)) !== null) {
    callbacks.add(match[1]);
  }
  return Array.from(callbacks).sort();
}

function normalizeCriteriaValue(
  value: string,
  criteria: EventCriteriaKind
): string | number | undefined {
  const trimmed = value.trim();
  if (!trimmed.length || criteria === "none") {
    return undefined;
  }
  if (criteria === "string") {
    return trimmed;
  }
  const numeric = Number(trimmed);
  if (Number.isFinite(numeric)) {
    return Math.trunc(numeric);
  }
  return criteria === "intOrString" ? trimmed : undefined;
}

function resolveDefaultCriteria(info: EventTypeInfo | undefined) {
  if (!info || !info.defaultCriteria) {
    return undefined;
  }
  return normalizeCriteriaValue(info.defaultCriteria, info.criteria);
}

export function EventBindingsView({
  entityType,
  events,
  script,
  canEdit,
  onChange
}: EventBindingsViewProps) {
  const availableEventTypes = useMemo(
    () => getEventTypesForEntity(entityType),
    [entityType]
  );
  const callbackOptions = useMemo(() => {
    const callbacks = new Set(extractCallbacks(script));
    for (const info of availableEventTypes) {
      callbacks.add(info.defaultCallback);
    }
    return Array.from(callbacks).sort();
  }, [availableEventTypes, script]);
  const triggerOptions = useMemo(() => {
    const known = new Set(availableEventTypes.map((info) => info.trigger));
    const extraTriggers = events
      .map((event) => event.trigger)
      .filter((trigger) => trigger && !known.has(trigger));
    const extras = Array.from(new Set(extraTriggers)).map((trigger) => ({
      trigger,
      label: `Unknown (${trigger})`
    }));
    const knownOptions = availableEventTypes.map((info) => ({
      trigger: info.trigger,
      label: info.trigger
    }));
    return [...extras, ...knownOptions];
  }, [availableEventTypes, events]);

  const handleUpdateEvent = (
    index: number,
    updater: (event: EventBinding) => EventBinding
  ) => {
    const nextEvents = events.map((event, eventIndex) =>
      eventIndex === index ? updater(event) : event
    );
    onChange(nextEvents);
  };

  const handleTriggerChange = (index: number, trigger: string) => {
    handleUpdateEvent(index, (event) => {
      const nextInfo = eventTypeByTrigger.get(trigger);
      const prevInfo = eventTypeByTrigger.get(event.trigger);
      const shouldResetCallback =
        !event.callback || event.callback === prevInfo?.defaultCallback;
      const nextCallback =
        shouldResetCallback && nextInfo?.defaultCallback
          ? nextInfo.defaultCallback
          : event.callback;
      const nextCriteria =
        nextInfo?.criteria === "none"
          ? undefined
          : event.criteria ?? resolveDefaultCriteria(nextInfo);
      return {
        ...event,
        trigger,
        callback: nextCallback,
        criteria: nextCriteria
      };
    });
  };

  const handleCallbackChange = (index: number, value: string) => {
    handleUpdateEvent(index, (event) => ({
      ...event,
      callback: value
    }));
  };

  const handleCriteriaChange = (index: number, value: string) => {
    handleUpdateEvent(index, (event) => {
      const info = eventTypeByTrigger.get(event.trigger);
      const criteria = normalizeCriteriaValue(
        value,
        info?.criteria ?? "string"
      );
      if (criteria === undefined) {
        const { criteria: _criteria, ...rest } = event;
        return rest;
      }
      return {
        ...event,
        criteria
      };
    });
  };

  const handleRemove = (index: number) => {
    const nextEvents = events.filter((_, eventIndex) => eventIndex !== index);
    onChange(nextEvents);
  };

  const handleAddEvent = () => {
    const fallback = availableEventTypes[0];
    if (!fallback) {
      return;
    }
    const nextEvent: EventBinding = {
      trigger: fallback.trigger,
      callback: fallback.defaultCallback
    };
    const criteria = resolveDefaultCriteria(fallback);
    if (criteria !== undefined) {
      nextEvent.criteria = criteria;
    }
    onChange([...events, nextEvent]);
  };

  if (!canEdit) {
    return (
      <div className="event-panel event-panel__empty">
        <h3>No event target selected</h3>
        <p>Select a room, mobile, or object to edit event bindings.</p>
      </div>
    );
  }

  return (
    <div className="event-panel">
      <div className="event-panel__header">
        <div>
          <div className="event-panel__title">Event Bindings</div>
          <div className="event-panel__meta">
            {events.length
              ? `${events.length} event${events.length === 1 ? "" : "s"} bound`
              : "No events bound yet"}
          </div>
        </div>
        <button
          className="action-button"
          type="button"
          onClick={handleAddEvent}
          disabled={!availableEventTypes.length}
        >
          Add Event
        </button>
      </div>
      {events.length ? (
        <div className="event-table">
          {events.map((event, index) => {
            const info = eventTypeByTrigger.get(event.trigger);
            const criteriaKind = info?.criteria ?? "string";
            const criteriaValue =
              event.criteria === undefined || event.criteria === null
                ? ""
                : String(event.criteria);
            const criteriaPlaceholder =
              info?.criteriaHint ??
              (criteriaKind === "intOrString"
                ? "VNUM or keyword"
                : criteriaKind === "int"
                  ? "Number"
                  : criteriaKind === "string"
                    ? "Keyword"
                    : "None");
            return (
              <div className="event-row" key={`${event.trigger}-${index}`}>
                <div className="form-field">
                  <label
                    className="form-label"
                    htmlFor={`event-trigger-${index}`}
                  >
                    Trigger
                  </label>
                  <select
                    id={`event-trigger-${index}`}
                    className="form-select"
                    value={event.trigger}
                    onChange={(eventTarget) =>
                      handleTriggerChange(index, eventTarget.target.value)
                    }
                  >
                    {triggerOptions.map((option) => (
                      <option key={option.trigger} value={option.trigger}>
                        {option.label}
                      </option>
                    ))}
                  </select>
                </div>
                <div className="form-field">
                  <label
                    className="form-label"
                    htmlFor={`event-callback-${index}`}
                  >
                    Callback
                  </label>
                  <input
                    id={`event-callback-${index}`}
                    className="form-input"
                    type="text"
                    list="event-callback-options"
                    value={event.callback}
                    onChange={(eventTarget) =>
                      handleCallbackChange(index, eventTarget.target.value)
                    }
                    placeholder={info?.defaultCallback ?? "on_event"}
                  />
                </div>
                <div className="form-field">
                  <label
                    className="form-label"
                    htmlFor={`event-criteria-${index}`}
                  >
                    Criteria
                  </label>
                  <input
                    id={`event-criteria-${index}`}
                    className="form-input"
                    type={criteriaKind === "int" ? "number" : "text"}
                    placeholder={criteriaPlaceholder}
                    value={criteriaKind === "none" ? "" : criteriaValue}
                    onChange={(eventTarget) =>
                      handleCriteriaChange(index, eventTarget.target.value)
                    }
                    disabled={criteriaKind === "none"}
                  />
                </div>
                <div className="event-row__actions">
                  <button
                    className="ghost-button event-row__remove"
                    type="button"
                    onClick={() => handleRemove(index)}
                  >
                    Remove
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      ) : (
        <div className="event-panel__empty">
          <p>No triggers yet. Add one to wire script callbacks.</p>
        </div>
      )}
      <datalist id="event-callback-options">
        {callbackOptions.map((callback) => (
          <option key={callback} value={callback} />
        ))}
      </datalist>
    </div>
  );
}
