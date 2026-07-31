export function structuralKeyEqual<Key>(left: Key, right: Key): boolean {
  if (Object.is(left, right)) return true;
  if (typeof left === "object" && left !== null && "equals" in left) {
    const equals = (left as { readonly equals?: unknown }).equals;
    if (typeof equals === "function") return Boolean(equals.call(left, right));
  }
  return false;
}
