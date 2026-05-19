import React from "react";

type ContainerProps = React.PropsWithChildren<{
  className?: string;
}>;

const Container = ({ className = "", children }: ContainerProps) => {
  return <div className={`container p-8 mx-auto xl:px-0 ${className}`}>{children}</div>;
};

export default Container;
