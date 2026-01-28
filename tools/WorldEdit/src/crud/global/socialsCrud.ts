import type { ColDef } from "ag-grid-community";
import type { SocialDataFile, SocialDefinition } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

export type SocialRow = {
  index: number;
  name: string;
  noTarget: string;
  target: string;
  self: string;
};

type SocialCrudCreateResult = {
  socialData: SocialDataFile;
  index: number;
  name: string;
};

type SocialCrudDeleteResult = {
  socialData: SocialDataFile;
  deletedName: string;
};

export const buildSocialRows = (
  socialData: SocialDataFile | null
): SocialRow[] => {
  if (!socialData) {
    return [];
  }
  return socialData.socials.map((social, index) => {
    const noTargetCount = [social.charNoArg, social.othersNoArg].filter(
      (value) => typeof value === "string" && value.trim().length
    ).length;
    const targetCount = [
      social.charFound,
      social.othersFound,
      social.victFound
    ].filter((value) => typeof value === "string" && value.trim().length)
      .length;
    const selfCount = [social.charAuto, social.othersAuto].filter(
      (value) => typeof value === "string" && value.trim().length
    ).length;
    return {
      index,
      name: social.name ?? "(unnamed)",
      noTarget: `${noTargetCount}/2`,
      target: `${targetCount}/3`,
      self: `${selfCount}/2`
    };
  });
};

export const socialColumns: ColDef<SocialRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
  { headerName: "No Target", field: "noTarget", width: 120 },
  { headerName: "Targeted", field: "target", width: 120 },
  { headerName: "Self", field: "self", width: 100 }
];

export const createSocial = (
  socialData: SocialDataFile | null
): CrudResult<SocialCrudCreateResult> => {
  if (!socialData) {
    return crudFail("Load socials before creating socials.");
  }
  const nextIndex = socialData.socials.length;
  const newSocial: SocialDefinition = {
    name: `newsocial${nextIndex + 1}`,
    charNoArg: "",
    othersNoArg: "",
    charFound: "",
    othersFound: "",
    victFound: "",
    charAuto: "",
    othersAuto: ""
  };
  const nextSocialData = {
    ...socialData,
    socials: [...socialData.socials, newSocial]
  };
  return crudOk({
    socialData: nextSocialData,
    index: nextIndex,
    name: newSocial.name
  });
};

export const deleteSocial = (
  socialData: SocialDataFile | null,
  selectedIndex: number | null
): CrudResult<SocialCrudDeleteResult> => {
  if (!socialData) {
    return crudFail("Load socials before deleting socials.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a social to delete.");
  }
  const deletedName = socialData.socials[selectedIndex]?.name ?? "social";
  const nextSocials = socialData.socials.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    socialData: { ...socialData, socials: nextSocials },
    deletedName
  });
};
